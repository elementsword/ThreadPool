#include "handleClientMessageTask.h"
#include "../openssl/hash_util.h"
#include <fstream>
// 构造函数
handleClientMessageTask::handleClientMessageTask(int clientSocket, mysqlPool *sqlPool, int epollFd, int eventFd, std::shared_ptr<std::mutex> clientsMutex, std::unordered_map<int, clientInfo> &clients, std::shared_ptr<std::mutex> brokenClientsMutex, std::queue<int> &brokenClients, std::atomic<int> &personNumber)
    : clientSocket(clientSocket), sqlPool(sqlPool), epollFd(epollFd), eventFd(eventFd), clientsMutex(clientsMutex), clients(clients), brokenClientsMutex(brokenClientsMutex), brokenClients(brokenClients), personNumber(personNumber)
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
    MessageType type = stringToMessageType(JsonHelper::get_X<std::string>(j, "type"));
    switch (type)
    {
    case MessageType::EXIT:
    {
        std::string username = JsonHelper::get_X<std::string>(j, "username");
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
        std::string username = JsonHelper::get_X<std::string>(j, "username");
        std::string password = JsonHelper::get_X<std::string>(j, "msg");

        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
        // 创建 prepared statement
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("SELECT * FROM users WHERE username=?"));

        // 设置参数
        pstmt->setString(1, username); // 第1个问号 ?
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
                std::string storedHash = res->getString("password");
                std::string storedSalt = res->getString("salt");
                if (verifyPassword(password, storedSalt, storedHash))
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
                    personNumber.fetch_add(1);
                    std::lock_guard<std::mutex> lock(*clientsMutex); // 加锁 修改clientSocket状态
                    auto it = clients.find(clientSocket);
                    if (it != clients.end())
                    {
                        it->second.status = "login";
                        it->second.username = username;
                    }
                }
                else
                {
                    LOG_INFO("Login failed: wrong password.");
                    std::string reply = JsonHelper::make_json("login", "server", "false").dump();
                    send(clientSocket, reply.c_str(), reply.size(), 0);
                }
            }
        }
        else
        {
            LOG_INFO("Login failed: wrong username.");
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
        std::string username = JsonHelper::get_X<std::string>(j, "username");
        std::string password = JsonHelper::get_X<std::string>(j, "msg");

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
            std::string salt = generateSalt();
            std::string hash = hashPasswordWithSalt(password, salt);
            // 2. 用户不存在，插入新用户
            std::unique_ptr<sql::PreparedStatement> insertStmt(
                conn->prepareStatement("INSERT INTO users(username, password,salt) VALUES (?, ? ,?)"));
            insertStmt->setString(1, username);
            insertStmt->setString(2, hash);
            insertStmt->setString(3, salt);
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

    case MessageType::UPLOAD:
    {
        std::string username = JsonHelper::get_X<std::string>(j, "username");
        std::string fileInfo = JsonHelper::get_X<std::string>(j, "msg");
        json f = json::parse(fileInfo);
        std::cout << f << std::endl;
        std::string filename = JsonHelper::get_X<std::string>(f, "filename");
        size_t filesize = JsonHelper::get_X<int>(f, "filesize");
        std::string filemd5 = JsonHelper::get_X<std::string>(f, "md5");
        // 秒传判断
        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT id FROM files WHERE md5 = ? AND filesize = ? AND status = 'completed' LIMIT 1"));
        checkStmt->setString(1, filemd5);
        checkStmt->setInt64(2, filesize);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        if (res->next())
        {
            json j = JsonHelper::make_json("upload", "server", "exists");
            std::string data = j.dump();
            send(clientSocket, data.c_str(), data.length(), 0);
            return; // 直接返回，不插入
        }
        else
        {
            json j = JsonHelper::make_json("upload", "server", "not exists");
            send(clientSocket, j.dump().c_str(), j.dump().length(), 0);
            std::ofstream outfile("uploads/" + filename, std::ios::binary);
            if (!outfile.is_open())
            {
                std::cerr << "无法创建文件" << std::endl;
                return;
            }
            size_t totalReceived = 0;
            const size_t bufferSize = 4096;
            char buffer[bufferSize];

            while (totalReceived < filesize)
            {
                ssize_t bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
                if (bytesReceived <= 0)
                {
                    std::cerr << "接收失败或连接关闭" << std::endl;
                    break;
                }
                outfile.write(buffer, bytesReceived);
                totalReceived += bytesReceived;
            }

            outfile.close();

            // 验证 MD5（选做）
            std::string computedMd5 = calculateMD5("uploads/" + filename);
            if (computedMd5 == filemd5)
            {
                LOG_INFO("文件接收成功，校验通过。");
                std::unique_ptr<sql::PreparedStatement> insertStmt(
                    conn->prepareStatement("INSERT INTO files(filename, username,filesize,uploaded_size,md5,status) VALUES (?, ? ,?,?,?,?)"));
                insertStmt->setString(1, filename);
                insertStmt->setString(2, username);
                insertStmt->setBigInt(3, std::to_string(filesize));
                insertStmt->setString(4, std::to_string(filesize));
                insertStmt->setString(5, filemd5);
                insertStmt->setString(6, "completed");
                insertStmt->execute();
            }
            else
            {
                std::cerr << "文件校验失败！" << std::endl;
                std::remove(("uploads/" + filename).c_str());
            }
            break;
        }
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
        {"register", MessageType::REGISTER},
        {"upload", MessageType::UPLOAD}};

    auto it = typeMap.find(typeStr);
    return (it != typeMap.end()) ? it->second : MessageType::UNKNOWN;
}