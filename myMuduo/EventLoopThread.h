#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H
/*
 * one loop per thread
 * 每个EventLoopThread类都封装了一个EventLoop和一个Thread
 * 
 */
#include "Thread.h"
#include "nocopyable.h"

#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread : nocopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
        const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_; // 保存startLoop后创建的EventLoop地址，线程启动后，通过loop_操作EventLoop
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};
#endif