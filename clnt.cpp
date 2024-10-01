// client.cpp

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

constexpr int BUF_SIZE = 1024;

int main()
{
    std::string server_ip = "127.0.0.1";
    int port;
    std::string client_name;

    // std::cout << "서버 IP를 입력하세요: ";
    // std::cin >> server_ip;
    std::cout << "서버 포트를 입력하세요: ";
    std::cin >> port;
    std::cout << "사용할 클라이언트 이름을 입력하세요: ";
    std::cin >> client_name;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        std::cerr << "소켓 생성에 실패했습니다.\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    {
        std::cerr << "유효하지 않은 주소입니다.\n";
        return 1;
    }

    if (connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "서버 연결에 실패했습니다.\n";
        return 1;
    }

    std::cout << "서버에 연결되었습니다.\n";

    // 클라이언트 이름 전송
    send(client_socket, client_name.c_str(), client_name.length(), 0);

    bool bexit = false;
    std::string message;
    char buffer[BUF_SIZE];
    std::cin.ignore(); // 입력 버퍼 비우기

    // 서버로부터 메시지를 수신하는 스레드
    std::thread receive_thread([&]
                               {
        while (!bexit) {
            ssize_t bytes_received = recv(client_socket, buffer, BUF_SIZE, 0);
            if (bytes_received <= 0) {
                std::cerr << "서버가 연결을 끊었거나 오류가 발생했습니다.\n";
                break;
            }

            buffer[bytes_received] = '\0';
            std::cout << "\n" << buffer << "\n";
        }

    // 클라이언트 종료 시 스레드 정리
    if (receive_thread.joinable()) {
        receive_thread.join();
    }

    close(client_socket);
    exit(1);
    return 0; });

    // 클라이언트에서 서버로 메시지 전송
    while (!bexit)
    {
        std::cout << "메시지를 입력하세요: ";
        std::getline(std::cin, message);

        if (message == "exit")
        {
            bexit = true;
            // break;
        }

        // 이름을 포함한 메시지 형식: "이름: 메시지"
        std::string full_message = client_name + ": " + message;
        send(client_socket, full_message.c_str(), full_message.length(), 0);
    }
}
