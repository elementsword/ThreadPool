#include "mysqlPool.h"
mysqlPool *mysqlPool::_instance = nullptr;
std::mutex mysqlPool::_mutex;
mysqlPool::mysqlPool()
{
    initPool("127.0.0.1", "root", "123456", "test", 3306, 10);
}

mysqlPool::~mysqlPool()
{
}

mysqlPool *mysqlPool::getInstance()
{
    if (!_instance)
    {
        // 锁定 防止一次建立多个
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_instance)
        {
            _instance = new mysqlPool();
        }
    }
    return _instance;
}

std::shared_ptr<sql::Connection> mysqlPool::getConnection()
{
    
}

void mysqlPool::initPool(const std::string &host,
                         const std::string &user,
                         const std::string &password,
                         const std::string &database,
                         int port,
                         int poolSize)
{
    this->host = host;
    this->user = user;
    this->password = password;
    this->database = database;
    this->port = port;
    this->poolSize = poolSize;
    for (int i = 0; i < poolSize; ++i)
    {
        connectionQueue.push(createConnection());
    }
}