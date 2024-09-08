#ifndef ACCEPTOR_H
#define ACCEPTOR_H
/*
 * Acceptor类负责接收client的连接请求，包装sockfd为Channel对象，并将Channel对象加入到Poller中，
 * 运行在baseLoop中, 也就是mainReactor。
 */

#include "Channel.h"
#include "Socket.h"
#include "InetAddress.h"

#include <functional>

class EventLoop;

class Acceptor 
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& addr)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
    ~Acceptor();

    // 由TcpServer调用，设置回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();
    Acceptor* get() { return this; }
private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};
#endif // ACCEPTOR_H