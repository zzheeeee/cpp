#include <stdio.h>
#include <iostream>
#include <string>
#include <fmt/core.h>
//#include <fmt/format.h>
//#include <format>
#include <mariadb/conncpp.hpp>

class MariaDBConnector
{
public:
    void initialize();
    void finalize();
    bool connect(const char* host, const char* user, const char* passwd, int port = -1);
    bool query(const char* query_content);
    bool isConnected();

protected:
private:
    bool checkInitialized();

    sql::Driver* driver_ = nullptr;
    std::unique_ptr<sql::Connection> conn_ = nullptr;

    bool initialized_ = false;
    bool connected_ = false;
};

#ifndef MARIA_DB_TUTORIAL_MARIA_DB_CONNECTOR_H
#define MARIA_DB_TUTORIAL_MARIA_DB_CONNECTOR_H

#endif //MARIA_DB_TUTORIAL_MARIA_DB_CONNECTOR_H
