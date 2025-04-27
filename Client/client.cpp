#include "client.h"

// 构造
Client::Client(int port, const std::string &serverIp) : clientSocket(-1), serverIp(serverIp), port(port), isConnected(false), isLogin(false)
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
    // 先登录 再进行监听操作
    login();
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
    while (isConnected)
    {
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
                sendMessage(message);
            }
        }
    }
}

// 发送文字消息
void Client::sendMessage(const json &j)
{
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

// 接收消息
void Client::receiveMessage()
{
    char buffer[1024] = {0};
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    json j = JsonHelper::from_buffer(buffer, bytesReceived);
    std::cout << j << std::endl;
    std::string type = j["type"];
    if (type == "login")
    {
        if (j["msg"] == "true")
        {
            isLogin = true;
            std::cout << "1" << std::endl;
        }
        else if (j["msg"] == "false")
        {
            isLogin = false;
            std::cout << "2" << std::endl;
        }
    }
    else if (type == "text")
    {
        std::cout << j["from"] << "：" << j["msg"] << std::endl;
    }
    else if (type == "notice")
    {
        std::cout << j["msg"] << std::endl;
    }
    else if (type == "image_message")
    {
        std::cout << "" << std::endl;
    }
    else if (type == "exit")
    {
        closeConnection();
    }
    else
    {
        std::cerr << "未知类型: " << type << std::endl;
    }
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
    json j = JsonHelper::make_json("tom", "exit");
    std::string data = j.dump();
    size_t len = data.size();
    send(clientSocket, data.c_str(), len, 0);
}

void Client ::login()
{
    while (!isLogin)
    {
        // 获取用户输入的用户名和密码
        std::cout << "Enter username: ";
        std::getline(std::cin, username);
        std::cout << "Enter password: ";
        std::getline(std::cin, password);
        // 发送消息
        json j = JsonHelper::make_json("login", username, password);
        sendMessage(j);
        receiveMessage();
    }
}