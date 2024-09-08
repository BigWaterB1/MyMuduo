#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <memory>

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeOut = 1000000; // 1s

int createEvnetfd()
{
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0)
    {
        LOG_FATAL("eventfd create failed:%d \n", errno);
    }
    return eventfd;
}

EventLoop::EventLoop()
    : quit_(false),
      looping_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      wakeupFd_(createEvnetfd()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_);
    if(t_loopInThisThread) //这个线程中运行过两次EventLoop构造函数
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this; //一个线程一个eventloop
    }

    //设置wakeupfd感兴趣的事件以及回调操作
    wakeupChannel_->enableReading();
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this)); // TO FIGURE OUT
    //wakeupChannel_->setReadCallback(EventLoop::handleRead);

}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
    LOG_DEBUG("EventLoop %p of thread %d destructs", this, threadId_);
}

/*
 * 唤醒操作
 */
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %zd bytes instead of 8", n);
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    while(!quit_)
    {
        activeChannels_.clear();
        //
        pollReturnTime_ = poller_->poll(kPollTimeOut, &activeChannels_);
        for(Channel* channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p of thread %d stops looping", this, threadId_);
    looping_ = false;
}

//
void EventLoop::quit()
{
    //如果是在当前loop所属的线程中，loop不可能阻塞在loop()中，则直接退出loop
    quit_ = true;
    //如果不是在当前loop所属的线程中，loop有可能阻塞在loop()中，则唤醒loop线程
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else // 在非当前loop所属线程中执行runInLoop
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // emplace_back直接构造
        pendingFunctors_.emplace_back(std::move(cb));
    }

    // || callingPendingFunctors_ 表示
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        //callingPendingFunctors_ = true;
        wakeup();
    }
}

// 用于唤醒该loop所在的线程
// 向该loop的wakeupFd_写入一个数据，wakeupChannel_的回调函数handleRead()会被调用，从而唤醒loop线程
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %zd bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors; 
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors)
    {
        functor();
    }

    callingPendingFunctors_ = false;

}