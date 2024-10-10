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
    //底层真正创建一个线程，并在线程里运行用threadFunc()函数传入的回调函数，也就是运行threadFunc()
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

// 新线程运行的函数
// 1. 创建EventLoop对象
// 2. 调用用户传入的callback函数，传入loop对象
// 3. 进入事件循环，loop.loop() => Poller.poll()
// 4. 如果EventLoop退出loop()循环了，说明结束了，loop_ = nullptr
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的EventLoop，和线程一一对应，one loop per thread

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