#include "jsonhelper.h"

json JsonHelper::make_json(const std::string &type, const std::string &username, const std::string &msg)
{
    return {
        {"type", type},
        {"username", username},
        {"msg", msg}};
}



json JsonHelper::from_buffer(const char *buffer, size_t length)
{
    return json::parse(std::string(buffer, length));
}
