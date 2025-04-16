#include "jsonhelper.h"

// 生成登录请求 JSON
json JsonHelper::make_login(const std::string &user, const std::string &password)
{
    return {
        {"type", "login"},
        {"user", user},
        {"password", password}};
}

// 生成文本消息 JSON（注意：缺少 sender 参数，需后续处理）
json JsonHelper::make_text_msg(const std::string &sender, const std::string &message)
{
    return {
        {"type", "text"},
        {"from",sender},
        {"message", message}
        // 警告：缺少 sender 字段，get_sender() 将无法获取值！
    };
}

// 从 JSON 中提取消息类型
std::string JsonHelper::get_type(const json &j)
{
    return j.at("type").get<std::string>();
}

// 从 JSON 中提取发送者（假设 JSON 包含 sender 字段）
std::string JsonHelper::get_sender(const json &j)
{
    return j.at("sender").get<std::string>();
}

json JsonHelper::from_buffer(const char *buffer, size_t length)
{
    return json::parse(std::string(buffer, length));
}
json JsonHelper::make_exit_msg(const std::string& sender)
{
    return {
        {"type", "exit"},
        {"from",sender}
    };
}