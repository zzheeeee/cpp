// server.cpp
#include "maria_db_connector.h"
#include <sstream>
#include <mariadb/conncpp.hpp>

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

constexpr int BUF_SIZE = 1024;

struct ClientInfo
{
    int socket;
    std::string name;
    std::string address;
};

std::vector<ClientInfo> client_sockets; // 클라이언트 정보를 저장하는 벡터
std::mutex client_sockets_mutex;

void send_message_to_client(const std::string &client_name, const std::string &message)
{
    std::lock_guard<std::mutex> lock(client_sockets_mutex);
    auto it = std::find_if(client_sockets.begin(), client_sockets.end(), [&](const ClientInfo &info)
                           { return info.name == client_name; });

    if (it != client_sockets.end())
    {
        send(it->socket, message.c_str(), message.size(), 0);
        // std::cout << "클라이언트 (" << it->name << ")에게 메시지를 전송했습니다.\n";
    }
    else
    {
        std::cerr << "해당 이름의 클라이언트가 없습니다.\n";
    }
}

void handle_client(int client_socket, std::string client_address)
{
    char buffer[BUF_SIZE];

    // 클라이언트 이름 받기
    ssize_t bytes_received = recv(client_socket, buffer, BUF_SIZE, 0);
    if (bytes_received <= 0)
    {
        std::cerr << "클라이언트 (" << client_address << ") 이름 수신 실패.\n";
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0';
    std::string client_name = buffer;

    // 클라이언트 정보 추가
    {
        std::lock_guard<std::mutex> lock(client_sockets_mutex);
        client_sockets.push_back({client_socket, client_name, client_address});
    }
    std::cout << "(" << client_name << ")님이 연결되었습니다.\n";
    
    send_message_to_client(client_name, "환영합니다!");
    // 클라이언트와 메시지 주고받기
    while (true)
    {
        bytes_received = recv(client_socket, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0)
        {
            std::cout << "클라이언트 (" << client_name << ")님이 연결을 끊었거나 오류가 발생했습니다.\n";
            close(client_socket);

            // 벡터에서 해당 클라이언트 소켓 제거
            std::lock_guard<std::mutex> lock(client_sockets_mutex);
            client_sockets.erase(
                std::remove_if(client_sockets.begin(), client_sockets.end(), [&](const ClientInfo &info)
                               { return info.socket == client_socket; }),
                client_sockets.end());
            break;
        }

        buffer[bytes_received] = '\0'; // 받은 데이터를 null로 종료
        std::cout << client_name << ": " << buffer << std::endl;
    }
}

void broadcast_message(const std::string &message)
{
    std::lock_guard<std::mutex> lock(client_sockets_mutex);
    for (const auto &client : client_sockets)
    {
        send(client.socket, message.c_str(), message.size(), 0);
    }
}

int main()
{
    bool bExit = false;
    std::cout << "Initialize program" << std::endl;
    MariaDBConnector maria;
    maria.initialize();

    std::string cmd_input = "10.10.20.116 dbd 1234 3306 ";

    while (true)
    {
        if (not maria.isConnected())
        {
            std::vector<std::string> words;
            std::istringstream iss(cmd_input);
            std::string word;
            while (std::getline(iss, word, ' '))
            {
                words.push_back(word);
            }

            if (words.size() == 3)
            {
                maria.connect(words[0].c_str(), words[1].c_str(), words[2].c_str());
                std::cout << std::endl;
                continue;
            }
            else if (words.size() == 4)
            {
                maria.connect(words[0].c_str(), words[1].c_str(), words[2].c_str(), std::stoi(words[3]));
                std::cout << std::endl;
                continue;
            }
        }
        std::cout << "DB연결 완료";
        break;
    }

    int port;
    std::cout << "서버가 리스닝할 포트를 입력하세요: ";
    std::cin >> port;
    std::cin.ignore(); // 입력 버퍼 비우기

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "소켓 생성에 실패했습니다.\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "바인딩에 실패했습니다.\n";
        return 1;
    }

    if (listen(server_socket, 3) < 0)
    {
        std::cerr << "리스닝에 실패했습니다.\n";
        return 1;
    }

    std::cout << "서버가 포트 " << port << "에서 리스닝 중입니다...\n";

    std::vector<std::thread> threads;

    // 클라이언트 연결 처리
    threads.emplace_back([&]
                         {
        while (!bExit) {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
            if (client_socket < 0) {
                std::cerr << "클라이언트 연결 수락에 실패했습니다.\n";
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            std::string client_address = client_ip;

            threads.emplace_back(handle_client, client_socket, client_address);

        }

    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    // DB 연결 해제
    std::cout << "Finalize program" << std::endl;
    maria.finalize();

    // 서버 종료
    close(server_socket);
    exit(1);
    return 0; });

    // 서버에서 메시지 입력받아 특정 클라이언트에게 전송 또는 전체 브로드캐스트
    while (!bExit)
    {
        std::string command;
        // std::cout << "명령어를 입력하세요 (broadcast, send, exit): ";
        std::getline(std::cin, command);

        if (command == "exit")
        {
            bExit = true;
            // std::cout << command;
            // break;
        }
        else if (command == "broadcast")
        {
            std::string server_message;
            std::cout << "방송할 메시지를 입력하세요: ";
            std::getline(std::cin, server_message);

            std::string broadcast_msg = "서버: " + server_message;
            broadcast_message(broadcast_msg);
        }
        else if (command == "send")
        {
            std::string client_name;
            std::cout << "메시지를 보낼 클라이언트 이름을 입력하세요: ";
            std::getline(std::cin, client_name);

            std::string client_message;
            std::cout << "보낼 메시지를 입력하세요: ";
            std::getline(std::cin, client_message);

            std::string specific_msg = "서버: " + client_message;
            send_message_to_client(client_name, specific_msg);
        }
        else
        {
            std::cerr << "알 수 없는 명령어입니다. 다시 시도해주세요.\n";
        }
    }

}
