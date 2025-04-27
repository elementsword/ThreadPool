#include "handleClientMessageTask.h"

// 构造函数
handleClientMessageTask::handleClientMessageTask(int clientSocket, std::vector<int> clients, mysqlPool *sqlPool, int epollFd) : clientSocket(clientSocket), clients(clients), sqlPool(sqlPool), epollFd(epollFd)
{
}

// 析构函数
handleClientMessageTask::~handleClientMessageTask()
{
}

void handleClientMessageTask::execute()
{

    char buffer[1024] = {0};
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0)
    {
        LOG_ERROR("read failed");
        // removeClient(clientSocket);
        return;
    }

    json j = JsonHelper::from_buffer(buffer, bytesRead);

    if (JsonHelper::get_X(j, "type") == "exit")
    {
        std::string i = JsonHelper::make_json("exit", "server").dump();
        send(clientSocket, i.c_str(), i.size(), 0);
        // removeClient(clientSocket);
        LOG_INFO("client" + JsonHelper::get_X(j, "from") + ": exit");
    }

    if (JsonHelper::get_X(j, "type") == "login")
    {
        std::string username = JsonHelper::get_X(j, "from");
        std::string password = JsonHelper::get_X(j, "msg");

        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
        // 创建 prepared statement
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM users WHERE username=? AND password=?"));
        // 设置参数
        pstmt->setString(1, username); // 第1个问号 ?
        pstmt->setString(2, password); // 第2个问号 ?
        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 验证 是否登录
        // 判断有没有找到
        if (res->next())
        {
            LOG_INFO("Login success!");
            // 查到了，账号密码正确
            std::string i = JsonHelper::make_json("login", "server","true").dump();
            send(clientSocket, i.c_str(), i.size(), 0);
        }
        else
        {
            LOG_INFO("Login failed: wrong username or password.");
            std::string i = JsonHelper::make_json("login", "server","false").dump();
            send(clientSocket, i.c_str(), i.size(), 0);
        }
    }

    if (JsonHelper::get_X(j, "type") == "text")
    {
        LOG_INFO("Received message from client " + std::to_string(clientSocket));

        std::string str = j.dump();
        for (int i : clients)
        {
            // 发送消息给其他客户端
            if (i != clientSocket)
            {
                send(i, str.c_str(), str.size(), 0);
            }
        }
    }
    // 在 Task 最后处理完数据之后，重新激活这个 fd
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = clientSocket;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientSocket, &ev);
}