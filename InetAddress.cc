#include"InetAddress.h"

#include<strings.h> //htons() bzero()
#include<string.h> //strlen()

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
};

InetAddress::InetAddress(const sockaddr_in& addr)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = addr.sin_family;
    addr_.sin_port = addr.sin_port;
    addr_.sin_addr.s_addr = addr.sin_addr.s_addr;
};

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    //sockets::toIpPort(buf, sizeof buf, getSockAddr());
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    in_port_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
};

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
};

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
};

const sockaddr_in* InetAddress::getSockAddr() const
{
    return &addr_;
};

// #include<iostream>
// int main(){
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
// }