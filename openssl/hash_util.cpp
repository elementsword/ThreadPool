#define OPENSSL_API_COMPAT 0x10100000L
#include "hash_util.h"
#include <openssl/sha.h>
#include <random>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/md5.h>
#include <sstream>
std::string generateSalt(size_t length)
{
    // 定义可用字符集（大小写字母 + 数字）
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    // 使用随机设备和 Mersenne Twister 生成器生成随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    std::string salt;
    for (size_t i = 0; i < length; i++)
    {
        salt += charset[dis(gen)];
    }
    return salt;
}
std::string sha256(const std::string &data)
{
    unsigned char hash[SHA256_DIGEST_LENGTH]; // 32 字节哈希输出
    // 调用 OpenSSL 的 SHA256 函数计算哈希
    SHA256(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);

    std::stringstream ss;
    // 将二进制哈希转换为十六进制字符串
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}
std::string hashPasswordWithSalt(const std::string &password, const std::string &salt)
{
    return sha256(password + salt); // 注意顺序：密码 + salt
}

// 验证密码是否正确
bool verifyPassword(const std::string &inputPassword, const std::string &storedSalt, const std::string &storedHash)
{
    // 根据输入的密码和存储的盐值计算哈希
    std::string inputHash = hashPasswordWithSalt(inputPassword, storedSalt);

    // 比较计算出来的哈希与存储的哈希
    return inputHash == storedHash;
}
//md5计算 
std::string calculateMD5(const std::string &filepath)
{
    // 打开文件
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件计算MD5" << std::endl;
        return "";
    }

    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    // 读取文件数据并更新 MD5
    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
    {
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    // 完成 MD5 计算
    MD5_Final(hash, &md5Context);

    // 转换 MD5 为十六进制字符串
    std::stringstream md5StringStream;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        md5StringStream << std::setw(2) << std::setfill('0') << std::hex << (int)hash[i];
    }

    return md5StringStream.str();
}