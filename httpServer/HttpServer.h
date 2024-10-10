#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "HttpContext.h"

#include <myMuduo/TcpServer.h>
#include <myMuduo/Logger.h>
#include <functional>
#include <unordered_map>

class HttpRequest;
class HttpResponse;
//class HttpContext;


/*
 * Http 服务器类
 * 对TcpServer类进行一层封装
*/
class HttpServer
{
public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;
    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr, 
               const std::string& name, 
               TcpServer::Option option = TcpServer::kNoReusePort);

    void setHttpCallback(const HttpCallback& cb) { HttpCallback_ = cb; }

    void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

    //EventLoop* getLoop() const { return server_.getLoop(); }

    void start();

private:

    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf, 
                   Timestamp receiveTime);
    
    void onRequest(const TcpConnectionPtr& conn, const HttpRequest& request);
    void onConnection(const TcpConnectionPtr& conn);

    TcpServer server_;
    HttpCallback HttpCallback_; // 由用户自定义的回调函数， 默认返回404

    using ConextManager = std::unordered_map<std::string, std::unique_ptr<HttpContext>>;
    using ContextMap = std::unordered_map<std::string, HttpContext*>;
    ConextManager contextManger_; //利用智能指针管理context的生命周期
    ContextMap contexts_; // HttpServer管理的所有活的Http连接都有一个context
};
#endif // HTTPSERVER_H