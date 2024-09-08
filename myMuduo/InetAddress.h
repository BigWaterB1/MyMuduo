#ifndef INETADDRESS_H
#define INETADDRESS_H

#include<netinet/in.h>
#include<arpa/inet.h> //inet_addr
#include<string>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr);

    std::string toIpPort() const;
    std::string toIp() const;
    uint16_t toPort() const;

    const struct sockaddr_in* getSockAddr() const;
     // { return sockets::sockaddr_cast(&addr6_); }
     void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};

#endif