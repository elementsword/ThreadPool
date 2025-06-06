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
        fileInfo file;
        file.username = JsonHelper::get_X<std::string>(j, "username");
        std::string msg = JsonHelper::get_X<std::string>(j, "msg");
        json f = json::parse(msg);
        std::cout << f << std::endl;
        file.filename = JsonHelper::get_X<std::string>(f, "filename");
        file.filesize = JsonHelper::get_X<size_t>(f, "filesize");
        file.md5 = JsonHelper::get_X<std::string>(f, "md5");
        std::string fileAddress = "../upload/" + file.filename;
        // 秒传判断
        std::shared_ptr<sql::Connection> conn = sqlPool->getConnection();
        std::unique_ptr<sql::PreparedStatement> checkStmt(
            conn->prepareStatement("SELECT id,uploaded_size,status,filename FROM files WHERE md5 = ? AND filesize = ? LIMIT 1"));
        checkStmt->setString(1, file.md5);
        checkStmt->setInt64(2, file.filesize);
        std::unique_ptr<sql::ResultSet> res(checkStmt->executeQuery());
        size_t totalReceived = 0;
        if (res->next())
        {
            file.uploaded_size = res->getUInt64("uploaded_size");
            file.status = res->getString("status");
            // 急速秒传
            if (file.status == "completed" && file.filesize == file.uploaded_size)
            {
                json fileInfoJson = JsonHelper::to_json(file);
                json j = JsonHelper::make_json("upload", "server", (fileInfoJson.dump()));
                std::string data = j.dump();
                send(clientSocket, data.c_str(), data.length(), 0);
                break; // 直接返回，不插入
            }
            // 断点续传 //待完成
            else if (file.status == "uploading" && file.filesize > file.uploaded_size)
            {
                json fileInfoJson = JsonHelper::to_json(file);
                json j = JsonHelper::make_json("upload", "server", (fileInfoJson.dump()));
                std::string data = j.dump();
                send(clientSocket, data.c_str(), data.length(), 0);
                // 打开文件
                std::fstream outfile(fileAddress, std::ios::binary | std::ios::in | std::ios::out);
                if (!outfile.is_open())
                {
                    std::cerr << "无法创建文件" << std::endl;
                    break;
                }
                else
                {
                    outfile.seekg(file.uploaded_size);
                    totalReceived = file.uploaded_size;
                    const size_t bufferSize = 4096;
                    char buffer[bufferSize];
                    while (totalReceived < file.filesize)
                    {
                        ssize_t bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
                        if (bytesReceived <= 0)
                        {
                            std::cerr << "接收失败或连接关闭" << std::endl;
                            // notifyClientExit(clientSocket, brokenClientsMutex, brokenClients, eventFd);
                            break;
                        }
                        outfile.write(buffer, bytesReceived);
                        totalReceived += bytesReceived;
                        file.uploaded_size = totalReceived;
                        // 更新
                        // std::unique_ptr<sql::PreparedStatement> insertStmt(
                        //     conn->prepareStatement("UPDATE files SET uploaded_size=? WHERE filename=? AND username=?"));
                        // insertStmt->setUInt64(1, static_cast<uint64_t>(totalReceived));
                        // insertStmt->setString(2, (file.filename));
                        // insertStmt->setString(3, (file.username));
                        // insertStmt->execute();
                    }

                    outfile.close();
                }
            }
        }
        // 完整传输
        else
        {
            file.status = "not exist";
            json fileInfoJson = JsonHelper::to_json(file);
            json j = JsonHelper::make_json("upload", "server", fileInfoJson.dump());
            send(clientSocket, j.dump().c_str(), j.dump().length(), 0);
            std::ofstream outfile(fileAddress, std::ios::binary);
            if (!outfile.is_open())
            {
                std::cerr << "无法创建文件" << std::endl;
                break;
            }
            else
            {
                totalReceived = 0;
                const size_t bufferSize = 4096;
                char buffer[bufferSize];
                // 初始化
                std::unique_ptr<sql::PreparedStatement> insertStmt(
                    conn->prepareStatement("INSERT INTO files(filename, username,filesize,uploaded_size,md5,status) VALUES (?, ? ,?,?,?,?)"));
                insertStmt->setString(1, file.filename);
                insertStmt->setString(2, file.username);
                insertStmt->setUInt64(3, static_cast<uint64_t>(file.filesize));
                insertStmt->setUInt64(4, static_cast<uint64_t>(file.uploaded_size));
                insertStmt->setString(5, file.md5);
                insertStmt->setString(6, "uploading");
                insertStmt->execute();
                while (totalReceived < file.filesize)
                {
                    ssize_t bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
                    if (bytesReceived <= 0)
                    {
                        std::cerr << "接收失败或连接关闭" << std::endl;
                        // notifyClientExit(clientSocket, brokenClientsMutex, brokenClients, eventFd);
                        break;
                    }
                    outfile.write(buffer, bytesReceived);
                    totalReceived += bytesReceived;
                    file.uploaded_size = totalReceived;
                    // 初始化
                    // std::unique_ptr<sql::PreparedStatement> insertStmt(
                    //     conn->prepareStatement("UPDATE files SET uploaded_size=? WHERE filename=? AND username=?"));
                    // insertStmt->setUInt64(1, static_cast<uint64_t>(totalReceived));
                    // insertStmt->setString(2, (file.filename));
                    // insertStmt->setString(3, (file.username));
                    // insertStmt->execute();
                }

                outfile.close();
            }
        }
        //最后数据库处理 
        if (file.uploaded_size == file.filesize)
        {
            // 验证 MD5（选做）
            std::string computedMd5 = calculateMD5(fileAddress);
            if (computedMd5 == file.md5)
            {
                LOG_INFO("文件接收成功，校验通过。");
                std::unique_ptr<sql::PreparedStatement> insertStmt(
                    conn->prepareStatement("UPDATE files SET status = ?,uploaded_size = ? WHERE md5 = ? AND filename=?"));
                insertStmt->setString(1, "completed");
                insertStmt->setUInt64(2, static_cast<uint64_t>(file.uploaded_size));
                insertStmt->setString(3, file.md5);
                insertStmt->setString(4, (file.filename));
                insertStmt->execute();
            }
            else
            {
                std::cerr << "文件校验失败！" << std::endl;
                std::remove((fileAddress).c_str());
            }
            break;
        }
        else
        {
            // 更新
            std::unique_ptr<sql::PreparedStatement> insertStmt(
                conn->prepareStatement("UPDATE files SET status = ?,uploaded_size=? WHERE filename=? AND username=?"));
            insertStmt->setString(1, "uploading");
            insertStmt->setUInt64(2, static_cast<uint64_t>(file.uploaded_size));
            insertStmt->setString(3, (file.filename));
            insertStmt->setString(4, (file.username));
            insertStmt->execute();
        }
        // 未传完
        // else
        // {
        //     LOG_INFO("文件传输中断了,未传完。");
        //     std::unique_ptr<sql::PreparedStatement> insertStmt(
        //         conn->prepareStatement("UPDATE files SET status=? WHERE md5=? AND filename=?"));
        //     insertStmt->setString(1, "");
        //     insertStmt->setString(2, file.md5);
        //     insertStmt->setString(3, (file.filename));
        //     insertStmt->execute();
        // }
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
        {"register", MessageType::REGISTER},
        {"upload", MessageType::UPLOAD}};

    auto it = typeMap.find(typeStr);
    return (it != typeMap.end()) ? it->second : MessageType::UNKNOWN;
}