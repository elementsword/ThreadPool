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
    // 新增：任意结构体转 json
    template <typename T>
    static json to_json(const T &obj)
    {
        return json(obj); // 这里会调用自定义的 to_json 函数
    }

    // 新增：json 转任意结构体
    template <typename T>
    static T from_json(const json &j)
    {
        return j.get<T>(); // 这里会调用自定义的 from_json 函数
    }
};
#endif // __JSON_H__
