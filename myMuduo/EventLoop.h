#ifndef EVENTLOOP_H
#define EVENTLOOP_H

/** 
 * 事件循环 主要包含两个模块: Channel Poller(epoll)
 */

#include "nocopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

//#include "Channel.h"
//#include "Poller.h"

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>

class Channel;
class Poller;

class EventLoop : nocopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop(); // 启动loop循环
    void quit(); // 退出loop循环

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    void runInLoop(Functor cb); // 在当前loop中执行回调函数
    void queueInLoop(Functor cb); // 把cb放入队列中，唤醒loop所在的线程，执行cb

    void wakeup(); // 唤醒子loop所在的线程

    // 调用Poller相关的方法
    void updateChannel(Channel* channel); 
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel); // 判断channel是否存在于loop中

    // 一个Eventloop对应一个thread
    // 判断当前是否在Eventloop的线程中
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}

private:
    void handleRead(); // wake up后，处理epoll事件
    void doPendingFunctors(); // 执行待执行的回调函数

    using ChannelList = std::vector<Channel*>;

    std::atomic<bool> quit_; // 标识退出loop循环
    std::atomic<bool> looping_; // 标识当前是否在loop循环

    const pid_t threadId_; // 记录当前evnetloop的线程id
    Timestamp pollReturnTime_; // poller返回发生事件的channel的时间
    std::unique_ptr<Poller> poller_; // loop使用的poller

    int wakeupFd_; // 用于唤醒sonloop的fd
    std::unique_ptr<Channel> wakeupChannel_; // 用于唤醒sonloop的channel

    ChannelList activeChannels_; // 存储loop下所有的channel
    Channel* currentActiveChannel_; // 当前正在处理的channel

    std::atomic<bool> callingPendingFunctors_; // 标识当前loop是否有待执行的函数
    std::vector<Functor> pendingFunctors_; // 存放loop待执行所有的回调函数
    std::mutex mutex_; // 互斥锁，用于保护pendingFunctors_的线程安全操作

};

#endif