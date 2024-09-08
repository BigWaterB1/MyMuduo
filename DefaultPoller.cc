/*
 * 为什么需要一个单独的DefaultPoller来实现newDefaultPoller这个方法？
 * 原因是Poller.cc中实现这个方法，需要include PollPoller.h / EpollPoller.h，
 * Poller是一个基类，派生类可以去引用它，但是它最好不要去引用一个派生类
 * 这不符合设计的逻辑，依赖关系得正确
 */
#include "Poller.h"
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
    if(::getenv("MUDUO_USE_EPOLL"))
    {
        return nullptr; // TODO: PollPoller
    }
    else
    {
        return new EPollPoller(loop);
    }
};