#include "maria_db_connector.h"

void MariaDBConnector::initialize()
{
    if (not initialized_)
    {
        try
        {
            driver_ = sql::mariadb::get_driver_instance();
            initialized_ = true;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error during initialization: " << e.what() << std::endl;
            return;
        }
    }
}

void MariaDBConnector::finalize()
{
    if (isConnected())
    {
        conn_->close();
    }

    driver_ = nullptr;
    connected_ = false;
}

bool MariaDBConnector::connect(const char *host, const char *user, const char *passwd, int port)
{
    if (checkInitialized())
    {
        try
        {
            sql::SQLString url;
            if (port < 0)
            {
                // url = fmt::print("{}{}", "jdbc:mariadb://", std::string(host));
                url = "jdbc:mariadb://10.10.20.116:3306/card_project";
            }
            else
            {
                url = "jdbc:mariadb://10.10.20.116:3306/card_project";
                // url = fmt::print("{}{}/{}", "jdbc:mariadb://", std::string(host), std::to_string(port));
            }

            sql::Properties properties({{"user", user}, {"password", passwd}});

            conn_.reset(driver_->connect(url, properties));
            connected_ = true;

            std::cout << "Connected to " << url << std::endl;

            return true;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error during connection: " << e.what() << std::endl;

            return false;
        }
    }

    return false;
}

bool MariaDBConnector::query(const char *query_content)
{
    if (checkInitialized())
    {
        try
        {
            std::unique_ptr<sql::Statement> stmt(conn_->createStatement());
            sql::SQLString query(fmt::format("{}", query_content));

            sql::ResultSet *res = stmt->executeQuery(query);
            if (res != nullptr)
            {

                if (res->next())
                {
                    // Print header for each column
                    unsigned int col_nums = res->getMetaData()->getColumnCount();
                    for (int i = 1; i <= col_nums; ++i)
                    {
                        std::cout << res->getMetaData()->getColumnName(i) << "\t";
                    }
                    std::cout << std::endl;

                    // Iterate through rows and print values
                    while (res->next())
                    {
                        for (int i = 1; i <= col_nums; ++i)
                        {
                            std::cout << res->getString(i) << "\t";
                        }
                        std::cout << std::endl;
                    }
                }
            }

            return true;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error during query: " << e.what() << std::endl;

            return false;
        }
    }
    return false;
}

bool MariaDBConnector::isConnected()
{
    return connected_;
}

bool MariaDBConnector::checkInitialized()
{
    if (initialized_)
    {
        return true;
    }
    else
    {
        std::cerr << "Maria DB is not initialized." << std::endl;
        return false;
    }
}