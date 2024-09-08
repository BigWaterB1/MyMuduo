#ifndef LOGGER_H
#define LOGGER_H 1

#include<string>

#include "nocopyable.h"

#define LOG_INFO(logmessageFormat, ...)\
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmessageFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)//do while(0)防止一些意想不到的错误

#define LOG_ERROR(logmessageFormat, ...)\
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmessageFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)

#define LOG_FATAL(logmessageFormat, ...)\
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmessageFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while(0)

#ifdef MUDUODEBUG
#define LOG_DEBUG(logmessageFormat, ...)\
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmessageFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0)
#else
#define LOG_DEBUG(logmessageFormat, ...)
#endif

enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};
class Logger : nocopyable
{
public:
    //获取日志唯一的实例对象
    //构造函数被私有化了
    static Logger& instance();

    void setLogLevel(int level);

    void log(std::string msg);
private:
    int logLevel_;
    Logger(){};//构造函数私有化
};

#endif