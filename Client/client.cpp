#include "client.h"

// 构造
Client::Client(int port, const std::string &serverIp) : clientSocket(-1), serverIp(serverIp), port(port)
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
    std::cout << "Connected to server: " << serverIp << ":" << port << std::endl;
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

    return std::string(buffer, sizeof(buffer));
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
    std::cerr << "Error: " << errorMessage << std::endl;
    closeConnection(); // 关闭连接
    exit(EXIT_FAILURE); // 退出程序
} 