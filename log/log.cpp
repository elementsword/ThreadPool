#include "log.h"

Log *Log::_instance = nullptr;
std::mutex Log::_mutex;
std::mutex Log::_init_mutex;
bool Log::IsInitialized = false;

Log *Log::getInstance()
{
}
// 构造
Log::Log() : root(log4cpp::Category::getRoot())
{
    if (!IsInitialized)
    {
        initialize();
    }
}

// 析构
Log::~Log()
{
    log4cpp::Category::shutdown();
}

// 初始化
void Log::initialize(const std::string &filename, log4cpp::Priority::Value level, bool consoleOutput, const std::string &pattern)
{
    // 锁 防止初始化多个
    std::lock_guard<std::mutex> lock(_init_mutex);
    if (IsInitialized)
    {
        return;
    }
    // 创建pattern
    log4cpp::PatternLayout *layout = new log4cpp::PatternLayout();
    // 设置日志格式
    layout->setConversionPattern(pattern);
    // 创建 FileAppender
    log4cpp::Appender *appender = new log4cpp::FileAppender("fileappender", filename);
    // 关联日志格式
    appender->setLayout(layout);
    // 添加到日志器
    root.addAppender(appender);
    // 是否启用控制台输出
    if (consoleOutput)
    {
        log4cpp::PatternLayout *consoleLayout = new log4cpp::PatternLayout();
        layout->setConversionPattern(pattern);
        //输出流
        log4cpp::Appender *consoleAppender = new log4cpp::OstreamAppender("consoleAppender", &std::cout);

        root.addAppender(consoleAppender);
    }
    // 设置日志等级
    root.setPriority(level);

    //已初始化
    IsInitialized=true;
}

void Log::log(log4cpp::Priority::Value level, const std::string& message){
    if(IsInitialized){
        root.log(level,message);
    }
}
void Log::debug(const std::string& message){
    log(log4cpp::Priority::DEBUG,message);
}
void Log::info(const std::string& message){
    log(log4cpp::Priority::INFO,message);
}
void Log::warn(const std::string& message){
    log(log4cpp::Priority::WARN,message);
}
void Log::error(const std::string& message){
    log(log4cpp::Priority::ERROR,message);
}
void Log::fatal(const std::string& message){
    log(log4cpp::Priority::FATAL,message);
}