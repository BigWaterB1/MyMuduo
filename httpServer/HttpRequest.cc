#include "HttpRequest.h"

#include <mutex>

#define likely(x) __builtin_expect(!!(x), 1)   // x很可能为真
#define unlikely(x) __builtin_expect(!!(x), 0) // x很可能为假

HttpRequest::HttpRequest()
        : method_(kInvalid),
          version_(kUnknown)
    {}

bool HttpRequest::setMethod(const char * start, const char * end)
{
    static std::map<string, Method> str2int;
    // static MutexLock init;
    static std::mutex mutex_; // 局部静态变量
    if (unlikely(str2int.empty()))
    {
        // MutexLockGuard lock(init);
        std::unique_lock<std::mutex> lock(mutex_);
        if (str2int.empty())
        {
            str2int = {
                {"GET", kGet},
                {"POST", kPost},
                {"HEAD", kHead},
                {"PUT", kPut},
                {"DELETE", kDelete}
            };
        }
    }
    string m(start, end);
    auto itr = str2int.find(m);
    method_ = itr == str2int.end() ? kInvalid : itr->second;
    return method_ != kInvalid;
}

void HttpRequest::addHeader(const char *start, const char *colon, const char *end)
{
    std::string field(start, colon);
    ++colon;
    // 去掉key - value之间的空格
    while(colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon, end);
    // 去掉value尾部的空格
    while(!value.empty() && isspace(value[value.size() - 1]))
    {
        value.resize(value.size() - 1);
    }
    headers_[field] = value;
}

const std::string HttpRequest::getHeader(const std::string& name) const
{
    std::string result;
    //std::map<std::string, std::string>::const_iterator it = headers_.find(name);
    auto it = headers_.find(name);
    if (it != headers_.end())
    {
        result = it->second;
    }
    return result;
}

void HttpRequest::swap(HttpRequest &that)
{
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
}