#ifndef __MYSQLPOOL_H__
#define __MYSQLPOOL_H__

#include <queue>
#include <mutex>
#include <string>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <memory>

class mysqlPool
{
public:
    // 得到池对象
    static mysqlPool *getInstance();
    // 得到一个对象
    std::shared_ptr<sql::Connection> getConnection();

private:
    mysqlPool();
    // 析构函数
    ~mysqlPool();
    void initPool(const std::string &host,
                  const std::string &user,
                  const std::string &password,
                  const std::string &database,
                  int port,
                  int poolSize);                                  // 初始化池
    static mysqlPool *_instance;                                  // 单例对象
    static std::mutex _mutex;                                     // 锁
    std::queue<std::shared_ptr<sql::Connection>> connectionQueue; // 连接队列
    std::shared_ptr<sql::Connection> createConnection(); //创建数据库连接
    std::string host, user, password, database;
    int port;
    int poolSize;
};
#endif // __MYSQLPOOL_H__
