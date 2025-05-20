// clientInfo.h
#ifndef FILEINFO_H
#define FILEINFO_H
#include <string>

// 客户端信息结构体
struct fileInfo
{
    std::string username;      // 用户名
    std::string filename;      // 文件名
    std::size_t filesize;      // 状态，如 "connected"、"disconnected"、"busy"
    std::size_t uploaded_size; // 已上传值
    std::string md5;           // 密码
    std::string status;        // 状态
};

// JSON 序列化
inline void to_json(nlohmann::json &j, const fileInfo &f)
{
    j = nlohmann::json{
        {"username", f.username},
        {"filename", f.filename},
        {"filesize", f.filesize},
        {"uploaded_size", f.uploaded_size},
        {"md5", f.md5},
        {"status", f.status}};
}
// 反序列化
inline void from_json(const nlohmann::json& j, fileInfo& f)
{
    j.at("username").get_to(f.username);
    j.at("filename").get_to(f.filename);
    j.at("filesize").get_to(f.filesize);
    j.at("uploaded_size").get_to(f.uploaded_size);
    j.at("md5").get_to(f.md5);
    j.at("status").get_to(f.status);
}
#endif // FILEINFO_H