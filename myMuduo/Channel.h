#ifndef CHANNEL_H
#define CHANNEL_H

/**
 * 理清楚 EventLoop、Channel、Poller之间的关系 
 * 它们在Reactor模型中对应 多路事件分发器(Demultiplex) 
 * Channel封装了sockfd和感兴趣的event, 如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */


#include "Timestamp.h"
#include "nocopyable.h"

#include <functional>
#include <memory>

class EventLoop;
// 只声明类型，因为没有用到方法，不需要include头文件

class Channel : nocopyable
{
public:
    //typedef std::function<void()> EventCallback; // 回调函数
    //typedef std::function<void(Timestamp)> ReadEventCallback;
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件的，调用相应的方法
    void handleEvent(Timestamp reveiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {readCallback_ = std::move(cb);}
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb) {closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb) {errorCallback_ = std::move(cb);}

    //防止channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const {return events_;}
    void set_revents(int revt) {revents_ = revt;}

    //返当前的fd事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    //设置fd相应感兴趣的事件状态 并调用update来调用epoll_ctl在epoll中注册感兴趣的事件
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    int index() { return index_; }
    void setIndex(int idx) { index_ = idx; }

    //
    EventLoop* ownerLoop() const {return loop_;}
    void remove(); //从事件循环中移除channel

private:
    void update(); //更新感兴趣的事件
    void handleEventWithGuard(Timestamp receiveTime); //处理事件，并保护回调函数的异常
private:

    //表示当前fd的状态
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; //所属的事件循环
    const int  fd_;   //fd，poller监听的对象
    int        events_; //注册fd感兴趣的事件
    int        revents_; // poller返回的具体事件
    int        index_; // used by Poller.
    bool       logHup_;

    std::weak_ptr<void> tie_; //跨线程的监视资源
    bool tied_;

    bool eventHandling_;
    bool addedToLoop_;

    //因为Channel通道里面能够获知fd最终发生的具体事件
    //所以它负责调用具体事件的回调操作
    //std::function<void()> readCallback_; //回调函数对象
    ReadEventCallback readCallback_;//用于绑定回调函数
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

#endif