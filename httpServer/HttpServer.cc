#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"

void defaultHttpCallback(const HttpRequest&, HttpResponse* response)
{
    // 404 表示请求的资源在服务器上不存在或未找到
    response->setStatusCode(HttpResponse::k404NotFound);
    response->setStatusMessage("Not Found");
    response->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop* loop,
            const InetAddress& listenAddr, 
            const std::string& name, 
            TcpServer::Option option)
            : server_(loop, listenAddr, name, option),
              HttpCallback_(defaultHttpCallback)
{
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, 
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::start()
{
    LOG_INFO("HttpServer[%s]::starts listening on %s", server_.name().c_str(), server_.ipPort().c_str());
    server_.start();
}

// 可读写事件回调
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf, 
                   Timestamp receiveTime)
{
    //HttpContext *context = static_cast<HttpContext>(conn->getMutableContext());
    LOG_INFO("HttpServer::onMessage");
    HttpContext* context = contexts_[conn->name()];

    if (!context->parseRequest(buf, receiveTime))
    {
        LOG_INFO("HttpServer::onMessage !context->parseRequest(buf, receiveTime)");
        LOG_INFO("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 已经收到完整的请求
    if (context->gotAll())
    {
        LOG_INFO("HttpServer::onMessage context->gotAll():%d", (int)context->gotAll());
        onRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& request)
{
    LOG_INFO("HttpServer::onRequest()");
    const string &connection = request.getHeader("Connection");
    bool close = connection == "close" ||
                 (request.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);
    HttpCallback_(request, &response);
    Buffer buf;
    //std::cout << "appendToBuffer()" << std::endl;
    response.appendToBuffer(&buf);
    //std::cout << "buffer:" << std::endl;
    std::cout << buf.toString() << std::endl;
    conn->send(&buf);
    //conn->send("yes");
    std::cout << "conn->send(buf.peek());" << std::endl;
    if (response.isCloseConnection())
    {
        conn->shutdown();
    }
}

// 连接建立或断开的回调函数
void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        //conn->setContext(HttpContext());
        HttpContext* c = new HttpContext();
        contexts_.emplace(conn->name(), c);
        contextManger_.emplace(conn->name(), std::unique_ptr<HttpContext>(c));
    }

    if(conn->disconnected())
    {
        contexts_.erase(conn->name());
        contextManger_.erase(conn->name()); // erase 会调用智能指针的析，从而释放资源
    }
}

