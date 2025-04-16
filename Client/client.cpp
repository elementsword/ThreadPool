#include "client.h"

// 构造
Client::Client(int port, const std::string &serverIp) : clientSocket(-1), serverIp(serverIp), port(port), isConnected(false)
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
                std::string message = receiveMessage();
                if (message.empty())
                {
                    std::cout << "Server closed connection." << std::endl;
                    closeConnection();
                    return;
                }
                std::cout << "Received message: " << message << std::endl;
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
// 发送消息
void Client::sendMessage(const std::string &message)
{
    // 发送消息
    ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
    if (bytesSent < 0)
    {
        handleError("send failed");
        return;
    }
    std::cout << "Sent message: " << message << std::endl;
}
// 接收消息
std::string Client::receiveMessage()
{
    char buffer[1024] = {0};
    // 发送消息
    ssize_t bytesReceive = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceive < 0)
    {
        handleError("send failed");
        return "";
    }

    return std::string(buffer, bytesReceive);
}
// 关闭连接
void Client::closeConnection()
{
    if (clientSocket != -1)
    {
        close(clientSocket); // 关闭 socket
        std::cout << "Socket closed successfully." << std::endl;
        clientSocket = -1; // 将 socket 置为无效值，防止多次关闭
    }
}
// 错误处理函数
void Client::handleError(const std::string &errorMessage)
{
    LOG_ERROR(errorMessage); // 记录错误日志
    closeConnection();       // 关闭连接
    exit(EXIT_FAILURE);      // 退出程序
}
// 优雅退出
void Client::exitNormal()
{
    std::string message("exit");
    sendMessage(message);
    while (receiveMessage() == std::string("success"))
    {
        std::cout<<"1"<<std::endl;
        closeConnection();
        isConnected = false;
    }
}