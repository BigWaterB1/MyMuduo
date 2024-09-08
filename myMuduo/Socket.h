#ifndef SOCKET_H
#define SOCKET_H

/*
 * 对socket fd的封装
 */

class InetAddress;

class Socket
{
public:
    Socket(int sockfd)
        : sockfd_(sockfd) 
    {}

    ~Socket();
    int fd() const { return sockfd_;}
    void bindAddress(const InetAddress& addr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    int sockfd_;
};

#endif // SOCKET_H