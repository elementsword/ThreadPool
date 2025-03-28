#include "server.h"

Server::Server(int port, int threadPoolSize)
    : port(port), threadPool(threadPoolSize)
{
    // 创建监听套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        LOG_ERROR("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定套接字
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);
    // 绑定失败
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        LOG_ERROR("bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // 监听套接字
    if (listen(serverSocket, SOMAXCONN) < 0)
    {
        LOG_ERROR("listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // 创建 epoll 实例
    epollFd = epoll_create1(0);
}

Server::~Server()
{
    close(serverSocket);
    close(epollFd);
}

// 处理新连接
void Server::handleNewConnection()
{
    // 接受新连接
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket < 0)
    {
        LOG_ERROR("accept failed");
        return;
    }

    // 将客户端套接字添加到 epoll 实例
    struct epoll_event event;
    // 监听文件描述符的可读事件
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) < 0)
    {
        LOG_ERROR("epoll_ctl failed");
        close(clientSocket);
        return;
    }

    // 添加客户端套接字到列表
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.push_back(clientSocket);
    LOG_INFO("New client connected: " + std::to_string(clientSocket));
}

void Server::removeClient(int clientSocket)
{
    // 从 epoll 实例中删除客户端套接字
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::vector<int>::iterator it = std::find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end())
    {
        clients.erase(it);
        close(clientSocket);
        LOG_INFO("Client disconnected: " + std::to_string(clientSocket));
    }
}
void Server::handleClientMessage(int clientSocket)
{
    char buffer[1024];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
    if (bytesRead <= 0)
    {
        LOG_ERROR("read failed");
        removeClient(clientSocket);
        return;
    }

    // 处理客户端消息
    std::string message(buffer, bytesRead);
    LOG_INFO("Received message from client " + std::to_string(clientSocket) + ": " + message);

    // 创建任务并提交到线程池
    Task *task = new calculateTask(1, 100); // 示例任务
    threadPool.submit(task);
}