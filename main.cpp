#include "maria_db_connector.h"
#include <sstream>
#include <mariadb/conncpp.hpp>

bool checkExitCommand(const std::string& str)
{
    if (str == "X")
    {
        return true;
    }
    return false;
}

int main()
{
    std::cout << "Initialize program" << std::endl;
    MariaDBConnector maria;
    maria.initialize();

    std::string cmd_input;

    while (true)
    {
        if (not maria.isConnected())
        {
            std::cout << "Enter host, user, password, port(optional) separated by space( )" << std::endl;
            std::cout << "Enter X to exit program." << std::endl;
            std::cout << ">> ";
            getline(std::cin, cmd_input);
            std::cout << std::endl;

            if(checkExitCommand(cmd_input))
            {
                break;
            }

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
        else
        {

            std::cout << "Enter query" << std::endl;
            std::cout << "Enter X to exit program." << std::endl;
            std::cout << ">> ";
            getline(std::cin, cmd_input);
            std::cout << std::endl;

            if(checkExitCommand(cmd_input))
            {
                break;
            }

            maria.query(cmd_input.c_str());
            std::cout << std::endl;
            continue;
        }
        std::cout << "Wrong input!" << std::endl;
    }
    std::cout << "Finalize program" << std::endl;
    maria.finalize();

    return 0;
}