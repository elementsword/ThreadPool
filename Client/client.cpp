#include "client.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "../log/log.h"
#include "../json/jsonhelper.h"
#include <vector>
#include <fstream>
#include <filesystem>
#include "../openssl/hash_util.h"
#include "../tools/tools.h"
#include "../Info/fileInfo.h"
// 构造
Client::Client(int port, const std::string &serverIp) : clientSocket(-1), serverIp(serverIp), port(port), isConnected(false), sqlPool(mysqlPool::getInstance())
{
    // 创建套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        handleError("socket failed");
        return;
    }

    // 设置服务器地址结构体
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);
}

// 析构
Client::~Client()
{
    closeConnection(); // 关闭连接
    std::cout << "Client closed." << std::endl;
}
// 连接服务器
void Client::connectToServer()
{
    // 连接服务器
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        handleError("connect failed");
        return;
    }
    isConnected = true;
    std::cout << "Connected to server: " << serverIp << ":" << port << std::endl;
    ui();
    epollFd = epoll_create1(0);
    if (epollFd < 0)
    {
        handleError("epoll_create1 failed");
        return;
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = clientSocket; // 监听客户端套接字
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) < 0)
    {
        handleError("epoll_ctl clientSocket failed");
        return;
    }
    event.data.fd = STDIN_FILENO; // 监听标准输入
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0)
    {
        handleError("epoll_ctl STDIN_FILENO failed");
        return;
    }
    std::cout << "epoll_ctl clientSocket and STDIN_FILENO success" << std::endl;
    // std::cout << "请输入信息：";
    while (isConnected)
    {
        std::cout << "请输入信息：";
        std::cout.flush();
        struct epoll_event events[2];
        // 等待事件发生 拷贝
        int numEvents = epoll_wait(epollFd, events, 2, -1);
        for (int i = 0; i < numEvents; ++i)
        {
            if (events[i].data.fd == clientSocket)
            {
                receiveMessage();
            }
            else if (events[i].data.fd == STDIN_FILENO)
            {
                std::string message;
                std::getline(std::cin, message);
                // 安全退出
                if (message == "exit")
                {
                    exitNormal();
                    continue;
                }
                // 上传
                if (message.substr(0, 7) == "upload ")
                {
                    std::string filepath = message.substr(7);
                    uploadFile(filepath);
                    continue;
                }
                sendMessage(message);
            }
        }
    }
}

// 发送文字消息
void Client::sendMessage(const std::string &message)
{
    if (!message.empty())
    {
        json j = JsonHelper::make_json("text", username, message);
        // 发送消息
        std::string data = j.dump();
        size_t len = data.size();
        ssize_t bytesSent = send(clientSocket, data.c_str(), len, 0);
        if (bytesSent < 0)
        {
            handleError("send failed");
            return;
        }
        LOG_INFO("Sent message: " + data);
    }
    // std::cout << "请输入信息：";
}

// 接收消息 登录成功之后的
void Client::receiveMessage()
{
    char buffer[1024] = {0};
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == 0)
    {
        // 服务器关闭连接
        handleError("服务器已关闭连接，客户端即将退出。\n");
        return;
    }
    else if (bytesReceived < 0)
    {
        handleError("接收消息失败 \n");
        return;
    }

    json j = JsonHelper::from_buffer(buffer, bytesReceived);
    std::string type = j["type"];
    if (type == "text")
    {
        std::cout << std::endl
                  << j["username"] << "：" << j["msg"] << std::endl;
    }
    else if (type == "notice")
    {
        std::cout << j["msg"] << std::endl;
    }
    else if (type == "image_message")
    {
        std::cout << "" << std::endl;
    }
    else
    {
        std::cerr << "未知类型: " << type << std::endl;
    }
    // std::cout << "请输入信息：";
}
// 关闭连接
void Client::closeConnection()
{
    if (clientSocket != -1)
    {
        close(clientSocket); // 关闭 socket
        std::cout << "Socket closed successfully." << std::endl;
        clientSocket = -1; // 将 socket 置为无效值，防止多次关闭
        isConnected = false;
    }
}
// 错误处理函数
void Client::handleError(const std::string &errorMessage)
{
    LOG_ERROR(errorMessage); // 记录错误日志
    closeConnection();       // 关闭连接
    exit(EXIT_FAILURE);      // 退出程序
}
// 优雅退出 真实退出自然不在这
void Client::exitNormal()
{
    std::string message("exit");
    json j = JsonHelper::make_json("exit", username, "");
    std::string data = j.dump();
    size_t len = data.size();
    send(clientSocket, data.c_str(), len, 0);
    closeConnection();
}

bool Client ::login()
{
    // 获取用户输入的用户名和密码
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    // 发送消息
    json j = JsonHelper::make_json("login", username, password);
    std::string data = j.dump();
    size_t len = data.size();
    ssize_t bytesSent = send(clientSocket, data.c_str(), len, 0);
    if (bytesSent < 0)
    {
        handleError("send failed");
        return false;
    }
    char buffer[1024] = {0};
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    j = JsonHelper::from_buffer(buffer, bytesReceived);
    std::cout << j << std::endl;
    std::string type = j["type"];
    if (type == "login")
    {
        if (j["msg"] == "true")
        {
            this->username = username;
            std::cout << "✅ Login successful!" << std::endl;
            return true;
        }
        else if (j["msg"] == "false")
        {
            std::cout << "❌ Login failed. Try again." << std::endl;
            return false;
        }
        else if (j["msg"] == "exist")
        {
            std::cout << "❌ Already login. Please change user." << std::endl;
            return false;
        }
    }
    return false;
}

void Client ::registerAccount()
{
    // 获取用户输入的用户名和密码
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);
    // 发送消息
    json j = JsonHelper::make_json("register", username, password);
    std::string data = j.dump();
    size_t len = data.size();
    ssize_t bytesSent = send(clientSocket, data.c_str(), len, 0);
    if (bytesSent < 0)
    {
        handleError("send failed");
        return;
    }
    char buffer[1024] = {0};
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    j = JsonHelper::from_buffer(buffer, bytesReceived);
    std::cout << j << std::endl;
    std::string type = j["type"];
    if (type == "register")
    {
        if (j["msg"] == "true")
        {
            std::cout << "✅ Registration successful. Please login." << std::endl;
        }
        else if (j["msg"] == "false")
        {
            std::cout << "❌ Registration failed: " << std::endl;
        }
    }
}

void Client::ui()
{
    while (true)
    {
        std::cout << "======================" << std::endl;
        std::cout << " 1. Login" << std::endl;
        std::cout << " 2. Register" << std::endl;
        std::cout << " 3. Exit" << std::endl;
        std::cout << "======================" << std::endl;
        std::cout << "Select option: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1")
        {
            if (login())
            {
                break;
            }
        }
        else if (choice == "2")
        {
            registerAccount();
        }
        else if (choice == "3")
        {
            std::cout << "👋 Exit. Goodbye!" << std::endl;
            exitNormal();
            exit(0);
        }
        else
        {
            std::cout << "⚠️ Invalid option. Try again.\n"
                      << std::endl;
        }
    }
}

void Client::uploadFile(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "文件地址错误,无法打开" << std::endl;
        return;
    }
    std::string filename = getFilenameFromPath(filepath);
    std::string filemd5 = calculateMD5(filepath);
    // 移动到末尾
    file.seekg(0, std::ios::end);
    // 计算大小
    size_t filesize = file.tellg();
    // 移动到头部
    file.seekg(0);
    json fileinfo;
    fileinfo["filename"] = filename;
    fileinfo["filesize"] = filesize;
    fileinfo["md5"] = filemd5;
    json j = JsonHelper::make_json("upload", username, fileinfo.dump());
    std::string data = j.dump();
    size_t len = data.size();
    ssize_t bytesSent = send(clientSocket, data.c_str(), len, 0);
    if (bytesSent < 0)
    {
        std::cout << "发送文件信息失败" << std::endl;
        file.close();
        return;
    }
    const size_t bufferSize = 4096;
    char buffer[bufferSize] = {0};
    if (recv(clientSocket, buffer, bufferSize, 0) < 0)
    {
        handleError("断开了...");
        return;
    }
    fileInfo recvFileInfo = json::parse(JsonHelper::get_X<std::string>(JsonHelper::from_buffer(buffer, bufferSize), "msg"));
    if (recvFileInfo.status == "completed")
    {
        file.close();
        std::cout << "急速秒传" << std::endl;
    }
    // 断点续传 //待完成
    else if (recvFileInfo.status == "uploading")
    {
        std::cout << "文件信息已发送，开始从中间传输文件..." << std::endl;
        // 分片传输
        buffer[bufferSize] = {0};
        file.seekg(recvFileInfo.uploaded_size);
        while (!file.eof())
        {
            file.read(buffer, bufferSize);
            std::streamsize bytesRead = file.gcount();
            if (bytesRead > 0)
            {

                ssize_t sent = send(clientSocket, buffer, bytesRead, 0);
                if (sent <= 0)
                {
                    std::cout << "发送文件内容失败" << std::endl;
                    break;
                }
                recvFileInfo.uploaded_size += sent;
                printProgressBar(static_cast<int>( (static_cast<double>(recvFileInfo.uploaded_size) / recvFileInfo.filesize) * 100 ));
            }
        }
        file.close();
        std::cout << "文件发送完成" << std::endl;
    }
    else
    {
        std::cout << "文件信息已发送，开始传输文件..." << std::endl;
        // 分片传输

        buffer[bufferSize] = {0};
        while (!file.eof())
        {
            file.read(buffer, bufferSize);
            std::streamsize bytesRead = file.gcount();
            if (bytesRead > 0)
            {
                ssize_t sent = send(clientSocket, buffer, bytesRead, 0);
                if (sent <= 0)
                {
                    std::cout << "发送文件内容失败" << std::endl;
                    break;
                }
                recvFileInfo.uploaded_size += sent;
                printProgressBar(static_cast<int>( (static_cast<double>(recvFileInfo.uploaded_size) / recvFileInfo.filesize) * 100 ));
            }
        }
        file.close();
        std::cout << "文件发送完成" << std::endl;
    }
    // std::cout << "请输入信息：";
}
