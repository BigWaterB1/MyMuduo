#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
    const std::string& name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb)
{
    std::cout << "EventLoopThread::EventLoopThread()" << std::endl;
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

// 开启后不主动结束，析构时自动结束
EventLoop* EventLoopThread::startLoop()
{
    //开启底层的新线程
    thread_.start();
    EventLoop* loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待loop_在threadFunc()中被赋值
        while(loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 由创建的thread运行
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的EventLoop，和上面的线程一一对应，one loop one thread

    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); //进入事件循环 EventLoop loop => Poller.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}