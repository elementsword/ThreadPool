#include "server.h"

Server::Server(int port, int threadPoolSize)
    : port(port), threadPool(threadPoolSize), sqlPool(mysqlPool::getInstance()), clientsMutex(std::make_shared<std::mutex>()),
      brokenClientsMutex(std::make_shared<std::mutex>())
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

    // 创建 eventfd
    eventFd = eventfd(0, EFD_NONBLOCK);
    if (eventFd == -1)
    {
        LOG_ERROR("eventfd failed");
        exit(EXIT_FAILURE);
    }

    // 创建 epoll 实例
    epollFd = epoll_create1(0);
}

Server::~Server()
{
    close(serverSocket);
    close(epollFd);
    close(eventFd);
}

void Server::start()
{
    // 将监听套接字添加到 epoll 实例
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) < 0)
    {
        LOG_ERROR("epoll_ctl failed serverSocket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    event.data.fd = eventFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &event) < 0)
    {
        LOG_ERROR("epoll_ctl failed eventFd");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    LOG_INFO("Server started on port " + std::to_string(port));
    personNumber = 0;
    while (true)
    {
        LOG_INFO("新的任务");
        // 等待事件发生
        struct epoll_event events[eventsSize];
        int numEvents = epoll_wait(epollFd, events, 10, 5000);
        if (numEvents < 0)
        {
            LOG_ERROR("epoll_wait failed");
            return;
        }
        // 超时 告诉所有用户 聊天室中有多少人
        else if (numEvents == 0)
        {

            noticeNumber();
            continue;
        }
        for (int i = 0; i < numEvents; ++i)
        {
            // 处理新链接
            if (events[i].data.fd == serverSocket)
            {
                handleNewConnection();
            }
            // eventFd收到信息 删除Client
            else if (events[i].data.fd == eventFd)
            {
                LOG_INFO("要删除了");
                uint64_t value;
                read(eventFd, &value, sizeof(value)); // 重要：清除事件
                removeClient();
            }
            // 处理信息
            else
            {
                handleClientMessage(events[i].data.fd);
            }
        }
    }
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
    event.events = EPOLLIN | EPOLLONESHOT;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) < 0)
    {
        LOG_ERROR("epoll_ctl failed");
        close(clientSocket);
        return;
    }

    // 添加客户端套接字到列表
    std::lock_guard<std::mutex> lock(*clientsMutex);
    clientInfo newClient;
    newClient.status = "connected";
    clients.insert({clientSocket, newClient});
    LOG_INFO("New client connected: " + std::to_string(clientSocket));
}

void Server::removeClient()
{

    Task *task = new removeTask(clientsMutex, clients, brokenClientsMutex, brokenClients, personNumber,sqlPool);
    threadPool.submit(task);
}

void Server::handleClientMessage(int clientSocket)
{
    Task *task = new handleClientMessageTask(clientSocket, sqlPool, epollFd, eventFd, clientsMutex, clients, brokenClientsMutex, brokenClients,personNumber);
    threadPool.submit(task);
}
//弃用
void Server::noticeNumber()
{
    // 创建任务并提交到线程池
    //Task *task = new noticeTask(personNumber, clients); // 示例任务
    //threadPool.submit(task);
}
