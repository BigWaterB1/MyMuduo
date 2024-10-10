#include "HttpServer.h"
#include "HttpResponse.h"
#include "HttpRequest.h"

char favicon[555] = {
  '\x89', 'P', 'N', 'G', '\xD', '\xA', '\x1A', '\xA',
  //太长，略，是一张图标的数据
};

void onRequest(const HttpRequest& req, HttpResponse* resp)
{
    bool benchmark = false;
    //std::cout << "Headers " << req.methodString() << " " << req.path() << std::endl;
    std::cout << "userSetHttpCallback:onRequest()" << std::endl;
    if (!benchmark)
    {
        const std::map<string, string>& headers = req.headers();
        for (const auto& header : headers)
        {
        //std::cout << header.first << ": " << header.second << std::endl;
        }
    }
    //按请求路径
    if (req.path() == "/") //请求行中的路径是 / 时
    {
        resp->setStatusCode(HttpResponse::k200Ok);//设置状态码
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        //resp->addHeader("Server", "Muduo");
        string now = Timestamp::now().toString();
        //设置应答实体body
        resp->setBody("<html><head><title>This is title</title></head>"
            "<body><h1>Hello</h1>Now is " + now +
            "</body></html>");
    }
    else if (req.path() == "/favicon.ico")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("image/png");
        resp->setBody(string(favicon, sizeof favicon));
    }
    else if (req.path() == "/hello")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        //resp->addHeader("Server", "Muduo");
        resp->setBody("hello, world!\n");
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
    }
    std::cout << "End of userSetHttpCallback:onRequest()" << std::endl;
}

int main() {
    int numThreads = 0;
    EventLoop loop;
    InetAddress addr(8020);
    HttpServer server(&loop, addr, "HttpServer");
    server.setHttpCallback(onRequest);
    server.setThreadNum(numThreads);
    server.start();
    loop.loop();
    return 0;
}
