#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

// ┌────────────────────────────────────────────────────────────────────────────────────────────────┐
// │                                              href                                              │
// ├──────────┬──┬─────────────────────┬────────────────────────┬───────────────────────────┬───────┤
// │ protocol │  │        auth         │          host          │           path            │ hash  │
// │          │  │                     ├─────────────────┬──────┼──────────┬────────────────┤       │
// │          │  │                     │    hostname     │ port │ pathname │     search     │       │
// │          │  │                     │                 │      │          ├─┬──────────────┤       │
// │          │  │                     │                 │      │          │ │    query     │       │
// "  https:   //    user   :   pass   @ sub.example.com : 8080   /p/a/t/h  ?  query=string   #hash "
// │          │  │          │          │    hostname     │ port │          │                │       │
// │          │  │          │          ├─────────────────┴──────┤          │                │       │
// │ protocol │  │ username │ password │          host          │          │                │       │
// ├──────────┴──┼──────────┴──────────┼────────────────────────┤          │                │       │
// │   origin    │                     │         origin         │ pathname │     search     │ hash  │
// ├─────────────┴─────────────────────┴────────────────────────┴──────────┴────────────────┴───────┤
// │                                              href                                              │
// └────────────────────────────────────────────────────────────────────────────────────────────────┘
// (All spaces in the "" line should be ignored. They are purely for formatting.)

#include <myMuduo/Timestamp.h>
#include <string>
#include <map>

/** Http 请求报文格式（CRLF表示回车换行 |表示空格）
 * 请求行：方法 | URL | 版本 CRLF
 * 首部行：首部字段名: | 值 CRLF
 * CRLF
 * 实体主体（通常不用）
 */
class HttpRequest {
public:
    enum Method
    {
        kInvalid,
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete
    };

    // HTTP 版本 只实现了 HTTP/1.0 和 HTTP/1.1
    enum Version
    {
        kUnknown,
        kHttp10,
        kHttp11
    };

    HttpRequest();

    bool setMethod(const char * start, const char * end);
    const Method getMethod() const { return method_; }

    void setVersion(const Version v) { version_ = v; }
    const Version getVersion() const { return version_; }

    void setPath(const char *start, const char *end) { path_.assign(start, end); }
    const std::string& path() const { return path_; }

    void setQuery(const char *start, const char *end) { query_.assign(start, end); }
    const std::string& query() const { return query_; }

    void setBody(const char *start, const char *end) { body_.assign(start, end); }
    const string &getBody() const { return body_; }

    void setReceiveTime(Timestamp t) { receiveTime_ = t; }
    const Timestamp receiveTime() const { return receiveTime_; }

    void addHeader(const char *start, const char *colon, const char *end);
    const std::string getHeader(const std::string& name) const;
    const std::map<std::string, std::string>& headers() const { return headers_; }

    void swap(HttpRequest &that);

private:
    Method method_;
    Version version_;
    std::string path_;
    std::string query_;
    std::string body_;
    Timestamp receiveTime_;
    std::map<std::string, std::string> headers_; // 请求头部
};

#endif // HTTPREQUEST_H