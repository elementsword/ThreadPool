#ifndef LOG_H
#define LOG_H

#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/PatternLayout.hh"
#include <string>
#include <mutex>
#include <atomic>

class Log {
public:
    // 获取单例实例（线程安全懒汉式）
    static Log* getInstance();

    // 初始化日志系统（仅允许调用一次）
    void initialize(
        const std::string& filename = "default.log",
        log4cpp::Priority::Value level = log4cpp::Priority::DEBUG,
        bool consoleOutput = false,
        const std::string& pattern = "%d [%p] %m%n");

    // 日志记录接口
    void log(log4cpp::Priority::Value level, const std::string& message);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    // 禁用拷贝和赋值
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

private:
    Log();
    ~Log();

    log4cpp::Category& root;
    // 静态成员变量
    static Log* _instance;
    static std::mutex _mutex;
    static std::mutex _init_mutex;
    static bool IsInitialized;
};

// 便捷宏定义
#define LOG_INIT(...)       Log::getInstance()->initialize(__VA_ARGS__)
#define LOG_DEBUG(message)  Log::getInstance()->debug(message)
#define LOG_INFO(message)   Log::getInstance()->info(message)
#define LOG_WARN(message)   Log::getInstance()->warn(message)
#define LOG_ERROR(message)  Log::getInstance()->error(message)
#define LOG_FATAL(message)  Log::getInstance()->fatal(message)

#endif // LOG_H
