#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>

EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpServer::ctor mainloop is nullptr", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             std::string nameArg,
             Option option)
        : loop_(loop)
        , ipPort_(listenAddr.toIpPort())
        , name_(nameArg)
        , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
        , threadPool_(new EventLoopThreadPool(loop, name_))
        , connectioncallback_()
        , messagecallback_()
        , nextConnId_(1)
        , started_(0)
{
    // 给acceptor设置新连接发生的回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
             std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto& item : connections_)
    {
        // 局部shared_ptr，出作用域后，可以自动释放new出来的TcpConnection对象资源
        TcpConnectionPtr conn = item.second;
        item.second.reset(); // TcpConnection.reset()
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

// 设置subloop的数量
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if(started_++ == 0) // 防止一个TcpServer被多次start
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 新用户连接，给acceptor_设置新用户连接发生时的回调函数
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    // 轮询获得的一个事件循环loop，如果没有设置线程数，那么就是mainloop
    EventLoop* ioloop = threadPool_->getNextLoop();
    LOG_DEBUG("TcpServer::newConnection ioLoop: %p", ioloop);

    // 获取连接名字connName
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_); // 名称
    ++nextConnId_; // 不涉及线程安全，只有一个线程在处理
    std::string connName = name_ + buf;

    LOG_INFO("TcpSever::newConnection [%s] - new connection [%s] from %s",
         name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());\
    
    // 通过getsockname获取本地地址 ip+端口
    sockaddr_in localAddr;
    ::bzero(&localAddr, sizeof localAddr);
    socklen_t addrlen = sizeof localAddr;
    if(::getsockname(sockfd, (sockaddr*)&localAddr, &addrlen) < 0)
    {
        LOG_ERROR("newConnection::getsockname");
    }
    InetAddress local_Addr(localAddr);
    
    TcpConnectionPtr conn( new TcpConnection(
                            ioloop,
                            connName,
                            sockfd,
                            local_Addr,
                            peerAddr
    )
    );
    connections_[connName] = conn;

    // 最终由用户设置
    conn->setConnectionCallback(connectioncallback_);
    conn->setMessageCallback(messagecallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // TcpServer::removeConnection()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    // 
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));// conn就是this
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // 到mainloop去执行，因为这个会由TcpConnection连接断开时触发调用
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// 给TcpConnection设置 最终会运行的连接断开时的回调函数
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection [%s]",
         name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}