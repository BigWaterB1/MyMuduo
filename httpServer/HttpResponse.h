#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <myMuduo/Buffer.h>
#include <map>
#include <string>


/*  HttpReponse 响应报文格式
 * （CRLF表示回车换行  |表示空格）
 * -------------------------------
 * 版本 | 状态码 | 短语 CRLF       状态行
 * 首部字段名: | 值 CRLF           首部行
 * CRLF
 * 实体主体（有些响应报文不用）
 * -------------------------------
 * 封装了构造响应报文的格式化过程
 */
class HttpResponse {
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k206PartialContent = 206,
        k301MovedPermanently = 301,
        k302Found = 302,
        k304NotModified = 304,
        k400BadRequest = 400,
        k403Forbidden = 403,
        k404NotFound = 404,
        k500InternalServerError = 500,
        k501NotImplemented = 501,
        k502BadGateway = 502,
        k503ServiceUnavailable = 503
    };

    explicit HttpResponse(bool close);
    void appendToBuffer(Buffer* output) const;
    std::string appendToStringBuffer() const;

    void setContentType(const std::string& contentType) { headers_["Content-Type"] = contentType; }
    void setStatusCode(const HttpStatusCode statuscode) { statuscode_ = statuscode; }
    void setStatusMessage(const std::string& statusMessage) { statusMessage_ = statusMessage; }
    void setBody(const std::string& body) { body_ = body; }

    void setCloseConnection(const bool on) { isCloseConnection_ = on; }
    bool isCloseConnection() const { return isCloseConnection_; }
    const int getFd() const { return fd_; }
    const off64_t getSendLen() const { return len_; }
    void setFd(const int fd) { fd_ = fd; }
    void setSendLen(const off64_t len) { len_ = len; }

    bool needSendFile() const { return fd_ != -1; }

private:
    std::map<std::string, std::string> headers_;  // 首部字段
    HttpStatusCode statuscode_;                   // 状态码
    std::string statusMessage_;                   // 状态短语
    std::string body_;                            // 实体主体
    bool isCloseConnection_;                      // 是否关闭长连接
    int fd_;                                      // 发送响应报文的套接字
    off64_t len_;                                 // 发送的字节数
};
#endif