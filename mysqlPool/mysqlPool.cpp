#include "mysqlPool.h"
mysqlPool *mysqlPool::_instance = nullptr;
std::mutex mysqlPool::_mutex;
std::mutex mysqlPool::queue_mutex;
std::condition_variable mysqlPool::cond;
mysqlPool::mysqlPool()
{
    initPool("127.0.0.1", "root", "123456", "test", 3306, 10);
}

mysqlPool::~mysqlPool()
{
    while (!connectionQueue.empty())
    {
        connectionQueue.pop();
    }
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
    std::unique_lock<std::mutex> lock(queue_mutex);
    // 条件变量 如果第一次未通过 即是空的队列 会休眠 释放锁 等待cond通知再拿锁
    cond.wait(lock, [this]()
              { return !connectionQueue.empty(); });
    auto conn = connectionQueue.front();
    connectionQueue.pop();
    // 返回的本质是conn.get
    // 自定义shared_ptr 析构函数 让析构时 放入队列中 同时通知 cond 去取
    return conn;
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

// 创建数据库连接
std::shared_ptr<sql::Connection> mysqlPool::createConnection()
{

    sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
    sql::Connection *conn = driver->connect("tcp://" + host + ":" + std::to_string(port), user, password);
    conn->setSchema(database);
    return std::shared_ptr<sql::Connection>(conn, [this](sql::Connection *ptr){
    std::lock_guard<std::mutex> lock(queue_mutex);
    connectionQueue.push(std::shared_ptr<sql::Connection>(ptr));
    return cond.notify_all(); });
}

//