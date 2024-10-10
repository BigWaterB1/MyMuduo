#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <unistd.h>

TcpConnection::TcpConnection(EventLoop* loop, 
                             const std::string& name,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
            : loop_(loop)
            , name_(name)
            , state_(kConnecting)
            , reading_(true)
            , socket_(new Socket(sockfd))
            , channel_(new Channel(loop, sockfd))
            , localAddr_(localAddr)
            , peerAddr_(peerAddr)
            , highWaterMark_(64*1024*1024) // 64M
{
    //TcpConnection给Channel设置回调函数，Poller监听到感兴趣的事件后，Channel会调用相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );
    socket_->setKeepAlive(true);
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
}

TcpConnection::~TcpConnection()
{
    int state = state_;
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), state);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0)
    {
        LOG_INFO("n > 0 TcpConnection::handleRead");
        // 这个是用户设置的
        // 已建立连接的用户，成功读到了发送来的消息，就调用用户传入的回调操作onMessage
        if(!messageCallback_) LOG_INFO("messageCallback_ is null");
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        LOG_INFO("n == 0 TcpConnection::handleRead handleClose()");
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        LOG_INFO("TcpConnection::handleWrite [%s] is writing\n", name_.c_str());
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    // 唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else{
        LOG_ERROR("TcpConnection::handleWrite but channel is not writing");
    }
}

// 连接断开事件的回调
void TcpConnection::handleClose()
{
    int state = state_;
    LOG_INFO("fd=%d state=%d", channel_->fd(), state);
    setState(kDisconncted);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this()); // shared_ptr不能直接由this指针构造，需要用shared_from_this()
    connectionCallback_(connPtr); // 执行用户设置的回调
    closeCallback_(connPtr); //这个是由TcpServer设置的回调函数，等于调用TcpServer::removeConnection()
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError [%s] SO_ERROR=%d\n", name_.c_str(), err);
}

void TcpConnection::send(Buffer* buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())// 在当前的线程
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(
                    fp,
                    this,
                    buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::send(const std::string& buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())// 在当前的线程
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(
                    fp,
                    this,
                    buf
                )
            );
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.c_str(), message.size());
}

/*
 * 应用发送速度大于内核发送速度，需要把数据写入缓冲区
 */
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 已经调用过shutdown
    if(state_ == kDisconncted)
    {
        LOG_ERROR("disconnected, give up writing!\n");
    }

    // channel_第一次写数据，而且缓冲区没有带发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0) // 发送成功
        {
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 当前发送并没有把数据全部发送出去，剩余数据需要保存到缓冲区去
    // 然后给channel注册epollout事件，poller发现tcp的发送缓冲区有空间
    // 就会同时相应的sock-channel，调用writeCallback_回调
    // 也就是调用TcpConnection::handleWrite()，把缓冲区的数据发送出去
    if(!faultError && remaining > 0)
    {
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + remaining >= highWaterMark_
            && oldlen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, 
                          shared_from_this(),   
                          oldlen + remaining
                )
            );
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册读事件为感兴趣事件

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected) // 某些情况下，直接调用TcpConnection::connectDestroyed() 才会进来，channel正常断开事件发生则不会进来
    {
        LOG_INFO("Uexpected! TcpConnection::connectDestroyed but state=%d\n", static_cast<int>(state_));
        setState(kDisconncted);
        channel_->disableAll(); // 从Poller中注销所有感兴趣事件
        closeCallback_(shared_from_this());
    }
    channel_->remove(); // 从Poller中移除channel
}

void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) // 说明当前outPutBuffer中的数据已经全部发送完了
    {
         socket_->shutdownWrite();
         // 会触发EPOLLHUB， 就会触发channel_的closeCallback_回调
         // 也就是，调用TcpConnection::handleClose()
    }
}