#include "handleClientMessageTask.h"

// 构造函数
handleClientMessageTask::handleClientMessageTask(int clientSocket, mysqlPool *sqlPool, int epollFd, int eventFd, std::shared_ptr<std::mutex> clientsMutex, std::unordered_map<int, clientInfo> &clients, std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int> &brokenClients)
    : clientSocket(clientSocket), sqlPool(sqlPool), epollFd(epollFd), eventFd(eventFd), clientsMutex(clientsMutex), clients(clients), brokenClientsMutex(brokenClientsMutex), brokenClients(brokenClients)
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
        notifyClientExit(clientSocket, brokenClientsMutex, brokenClients, eventFd);
        return;
    }

    json j = JsonHelper::from_buffer(buffer, bytesRead);
    MessageType type = stringToMessageType(JsonHelper::get_X(j, "type"));
    switch (type)
    {
    case MessageType::EXIT:
    {
        std::string username = JsonHelper::get_X(j, "username");
        std::string reply = JsonHelper::make_json("exit", "server").dump();
        send(clientSocket, reply.c_str(), reply.size(), 0);
        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
        std::unique_ptr<sql::PreparedStatement> updatePstmt(
            conn->prepareStatement("UPDATE users SET is_logged_in=0 WHERE username=?"));
        updatePstmt->setString(1, username);
        updatePstmt->execute();
        notifyClientExit(clientSocket, brokenClientsMutex, brokenClients, eventFd);
        break;
    }
    case MessageType::LOGIN:
    {
        std::string username = JsonHelper::get_X(j, "username");
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
            // 密码正确，检查是否已登录
            bool isLoggedIn = res->getBoolean("is_logged_in");
            if (isLoggedIn)
            {
                LOG_INFO("Already Login!");
                std::string reply = JsonHelper::make_json("login", "server", "exist").dump();
                send(clientSocket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                // 用户未登录，更新状态为登录
                std::unique_ptr<sql::PreparedStatement> updatePstmt(
                    conn->prepareStatement("UPDATE users SET is_logged_in=1 WHERE username=?"));
                updatePstmt->setString(1, username);
                updatePstmt->execute();
                LOG_INFO("Login success!");
                // 查到了，账号密码正确 并且未登录
                std::string reply = JsonHelper::make_json("login", "server", "true").dump();
                send(clientSocket, reply.c_str(), reply.size(), 0);
                std::lock_guard<std::mutex> lock(*clientsMutex); // 加锁 修改clientSocket状态
                auto it = clients.find(clientSocket);
                if (it != clients.end())
                {
                    it->second.status = "login";
                }
            }
        }
        else
        {
            LOG_INFO("Login failed: wrong username or password.");
            std::string reply = JsonHelper::make_json("login", "server", "false").dump();
            send(clientSocket, reply.c_str(), reply.size(), 0);
        }
        break;
    }
    case MessageType::TEXT:
    {
        LOG_INFO("Received message from client " + std::to_string(clientSocket));

        std::string str = j.dump();
        for (const auto &it : clients)
        {
            // 发送消息给其他客户端
            if (it.first != clientSocket)
            {
                send(it.first, str.c_str(), str.size(), 0);
            }
        }
        break;
    }
    case MessageType::REGISTER:
    {
        std::string username = JsonHelper::get_X(j, "username");
        std::string password = JsonHelper::get_X(j, "msg");

        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();

        // 1. 检查用户名是否已存在
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT * FROM users WHERE username=?"));
        checkStmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        if (res->next())
        {
            LOG_INFO("Register failed: user already exists.");
            std::string reply = JsonHelper::make_json("register", "server", "exists").dump();
            send(clientSocket, reply.c_str(), reply.size(), 0);
        }
        else
        {
            // 2. 用户不存在，插入新用户
            std::unique_ptr<sql::PreparedStatement> insertStmt(
                conn->prepareStatement("INSERT INTO users(username, password) VALUES (?, ?)"));
            insertStmt->setString(1, username);
            insertStmt->setString(2, password);
            int affectedRows = insertStmt->executeUpdate();
            // 更新行数 成功失败
            if (affectedRows > 0)
            {
                LOG_INFO("Register success!");
                std::string reply = JsonHelper::make_json("register", "server", "success").dump();
                send(clientSocket, reply.c_str(), reply.size(), 0);
            }
            else
            {
                LOG_ERROR("Register failed: no rows affected.");
                std::string reply = JsonHelper::make_json("register", "server", "fail").dump();
                send(clientSocket, reply.c_str(), reply.size(), 0);
            }
        }
        break;
    }
    default:
        LOG_WARN("Unknown message type received.");
        break; // 默认情况，处理未知类型的消息
    }

    // 在 Task 最后处理完数据之后，重新激活这个 fd
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = clientSocket;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientSocket, &ev);
}

// 通知要主线程要退出
void handleClientMessageTask::notifyClientExit(int clientSocket, std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int> &brokenClients, int eventFd)
{
    {
        // 1. 加锁，push到 brokenClients 队列
        std::lock_guard<std::mutex> lock(*brokenClientsMutex);
        brokenClients.push(clientSocket);
    }
    // 2. 通知主线程，写 eventFd
    uint64_t u = 1;
    write(eventFd, &u, sizeof(u));
    LOG_INFO("client: exit");
}

MessageType handleClientMessageTask::stringToMessageType(const std::string &typeStr)
{
    static const std::unordered_map<std::string, MessageType> typeMap = {
        {"exit", MessageType::EXIT},
        {"login", MessageType::LOGIN},
        {"text", MessageType::TEXT},
        {"register", MessageType::REGISTER}};

    auto it = typeMap.find(typeStr);
    return (it != typeMap.end()) ? it->second : MessageType::UNKNOWN;
}