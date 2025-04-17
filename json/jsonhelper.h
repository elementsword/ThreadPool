#ifndef __JSON_H__
#define __JSON_H__

#include <nlohmann/json.hpp>
using json = nlohmann::json;
class JsonHelper
{
public:
    static json from_buffer(const char *buffer, size_t length);
    static json make_json(const std::string &sender, const std::string &option, const std::string &msg = "");
    static std::string get_X(const json &j, const std::string &X);
};
#endif // __JSON_H__
