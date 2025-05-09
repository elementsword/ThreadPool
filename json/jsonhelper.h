#ifndef __JSON_H__
#define __JSON_H__

#include <nlohmann/json.hpp>
using json = nlohmann::json;
class JsonHelper
{
public:
    static json from_buffer(const char *buffer, size_t length);
    static json make_json(const std::string &type, const std::string &username, const std::string &msg = "");
    template <typename T>
    static T get_X(const nlohmann::json &j, const std::string &X)
    {
        return j.at(X).get<T>();
    }
};
#endif // __JSON_H__
