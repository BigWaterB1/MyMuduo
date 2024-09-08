#ifndef TCPSERVER_H
#define TCPSERVER_H
/*
 *  上层面向用户最主要的接口类
 *  封装了一个Acceptor accptor_ 和 TcpConnection 的容器 connections_
 */
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "nocopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             std::string nameArg,
             Option option = kNoReusePort);
    ~TcpServer();

    void setThreadNum(int numThreads); // 设置subloop的数量
    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectioncallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb)
    { messagecallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_; // baseloop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在 mianloop 监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectioncallback_;       // 有新连接的回调
    MessageCallback messagecallback_;             // 有读写消息发生的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完毕的回调

    ThreadInitCallback threadInitCallback_; // 线程初始化回调
    std::atomic<int> started_;

    int nextConnId_; // 连接id
    ConnectionMap connections_; // 保存所有的连接
};

#endif // TCPSERVER_H