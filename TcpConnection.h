#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "nocopyable.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>
#include <string>
#include <atomic>
#include <functional>
/*
 * TcpConnection:
 * TcpServer -> Acceptor -> 新用户连接, accept() -> connfd
 * 回调函数设置：用户-> TcpServer -> TcpConnection -> Channel -> Poller -> Channel的回调操作
 */

class Channel;
class EventLoop;
class Socket;

class TcpConnection : nocopyable,
    public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, 
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    //void send(const void* data, size_t len);
    void send(const std::string& buf);
    void shutdown();


    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
    { highWaterMarkCallback_ = cb; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    void connectEstablished();
    void connectDestroyed();

    void setState(int state)
    { state_ = state;}
private:
    enum state
    {
        kDisconncted,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();
    
    EventLoop* loop_; // 这里不是baseloop, 因为TcpConnection是baseloop管理的
    const std::string name_;
    std::atomic<int> state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完毕时的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_; // 高水位标记

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

#endif // TCPCONNECTION_H