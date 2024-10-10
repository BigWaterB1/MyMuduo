#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <memory>

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeOut = 10000; // 给epoll_wait设置的超时时间, 单位为微秒 milliseconds


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

    //wakeupChannel_只要对读事件刚兴趣就好了，注册到本loop所拥有的poller上
    wakeupChannel_->enableReading();
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
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
 * 调用一下read，这个无所谓，只是为了唤醒loop线程
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
        // 就是调用epoll_wait
        // 等待事件发生，返回活跃的channel
        // 超时时间为10s，wakeupChannel_的作用就是打断这个10秒钟
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
    //如果是在当前loop所属的线程中，EventLoop不可能阻塞在loop()中，会直接退出loop
    quit_ = true;
    //如果不是在当前loop所属的线程中，EventLoop有可能阻塞在loop()中，则唤醒loop线程
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 如果在当前loop所属的线程，那么直接执行函数
    {
        cb();
    }
    else // 如果不是在当前loop所属的线程中，调用queueInLoop
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    // 加锁，保证pendingFunctors_的修改安全
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // emplace_back直接构造
        // 不在自己的线程， 就把待执行的回调函数放到pendingFunctors_中
        // 因为所有的回调都要到自己的线程中执行
        pendingFunctors_.emplace_back(std::move(cb));
    }

    // || callingPendingFunctors_ 表示
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        //callingPendingFunctors_ = true;
        // 如果不是在自己的线程中，则唤醒loop线程
        wakeup();
    }
}

// 用于唤醒该loop所在的线程
// 向该loop的wakeupFd_随便写入一个数据，wakeupChannel_的回调函数handleRead()会被调用，从而唤醒loop线程
// 唤醒的含义就是eventLoop从epoll_wait()返回，从而处理事件，看loop()就知道了
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