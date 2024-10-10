#include "HttpResponse.h"

#include <string.h>

HttpResponse::HttpResponse(bool close)
    : statuscode_(kUnknown),
      isCloseConnection_(close),
      fd_(-1),
      len_(0)
{
}

void HttpResponse::appendToBuffer(Buffer* output) const
{
    char buf[32];
    bzero(&buf, sizeof buf);
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statuscode_);
    output->append(buf, sizeof buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if(isCloseConnection_)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        if(!needSendFile()) {
            bzero(&buf, sizeof buf);
            snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
            output->append(buf, sizeof buf);
        }
        output->append("Connection: Keep-Alive\r\n");
    }

    for(const auto& header : headers_){
        output->append(header.first);
        output->append(": ");
        output->append(header.second);  
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);
}

std::string HttpResponse::appendToStringBuffer() const
{
    return " ";
}