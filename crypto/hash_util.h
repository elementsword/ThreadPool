// hash_util.h
#ifndef HASH_UTIL_H
#define HASH_UTIL_H
#include <string>

std::string generateSalt(size_t length = 16);
std::string sha256(const std::string &data);
std::string hashPasswordWithSalt(const std::string &password, const std::string &salt);
bool verifyPassword(const std::string &inputPassword, const std::string &storedSalt, const std::string &storedHash);//验证
#endif // HASH_UTIL_H