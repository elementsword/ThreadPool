#include "noticeTask.h"
// 构造函数
noticeTask::noticeTask(int n, const std::unordered_map<int, clientInfo> &clients) : number(n), clients(clients)
{
    
}

// 析构函数
noticeTask::~noticeTask()
{
}

void noticeTask::execute()
{
    // 处理客户端消息
    std::string message = std::string("该服务器中还有") + std::to_string(number) + std::string("人。");
    std::string str = JsonHelper::make_json("text", "server", message).dump();
    for (const auto &it : clients)
    {
        if (it.second.status == "login")
        {
            send(it.first, str.c_str(), str.size(), 0);
        }
    }
}
