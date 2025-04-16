#ifndef __JSON_H__
#define __JSON_H__

#include <nlohmann/json.hpp>
using json = nlohmann::json;
class JsonHelper{
    public:
    static json make_login(const std::string& user, const std::string& password);
    static json make_text_msg(const std::string& sender,const std::string& message);
    static json from_buffer(const char* buffer, size_t length);
    static json make_exit_msg(const std::string& sender);
    static std::string get_type(const json& j);
    static std::string get_sender(const json& j);
};
#endif // __JSON_H__
