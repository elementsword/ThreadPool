#include "jsonhelper.h"

json JsonHelper::make_json(const std::string &type, const std::string &sender, const std::string &msg)
{
    return {
        {"type", type},
        {"from", sender},
        {"msg", msg}};
}

// 从 JSON 中提取发送者（假设 JSON 包含 sender 字段）
std::string JsonHelper::get_X(const json &j, const std::string &X)
{
    return j.at(X).get<std::string>();
}

json JsonHelper::from_buffer(const char *buffer, size_t length)
{
    return json::parse(std::string(buffer, length));
}
