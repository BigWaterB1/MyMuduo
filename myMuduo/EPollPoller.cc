#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <strings.h>
#include <unistd.h> // close()
#include <assert.h>

//channel未添加到epollpoller中，新创建的channel的index_是-1
const int kNew = -1;
//channel已添加到epollpoller中
const int kAdded = 1;
//channel已从epoolpoller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop)
  , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
  , events_(kInitEventListSize) // std::vector<epoll_event>(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 主要功能是向Poller注册channel
void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_DEBUG("func = %s =>fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel; // 保存channel到Poller的channels_中
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel); // 添加到epoll中
    }
    else//index == kadded == 1
    {
        int fd = channel->fd();
        //(void) fd;
        if(channel->isNoneEvent())
        {
            LOG_DEBUG("func = %s =>fd=%d events=%d index=%d isNoneEvent", __FUNCTION__, channel->fd(), channel->events(), index);
            update(EPOLL_CTL_DEL, channel); // 删除
            channel->setIndex(kDeleted); // 标记为删除, 但还是储存在channels_中
        }
        else
        {
            update(EPOLL_CTL_MOD, channel); // 修改
        }
    }
}

//从EPollPoller的ChannelMap channels_中移除channel
void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    int index = channel->index();
    channels_.erase(fd);
    LOG_DEBUG("func = %s =>fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if(index == kAdded)
    {
        LOG_DEBUG("EPOLL_CTL_DEL 删除");
        update(EPOLL_CTL_DEL, channel); // 删除
    }
    channel->setIndex(kNew); 
}

// 调用epoll_ctl函数，将channel注册到epoll中
//operation: EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD
void EPollPoller::update(int operation, Channel* channel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;//携带了fd

    int fd = channel->fd();
    LOG_DEBUG("EPollPoller::update fd:%d events:%d operation:%d channel:%p event.data.ptr:%p\n", fd, channel->events(), operation, channel, static_cast<Channel*>(event.data.ptr));
    //event.data.fd = fd; // 会出现覆盖问题
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        //出错处理
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

//EventLoop通过返回的activechannels知道哪些channel是活跃的
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    //poll函数的主要功能实现
    //events_接收epoll_wait的返回值，保存了活跃的channel的事件
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());//默认拷贝构造
    if(numEvents > 0)
    {
        LOG_DEBUG("func= %s => epoll_wait return numEvents:%d", __FUNCTION__, numEvents);
        fillActiveChannels(numEvents, activeChannels);
        LOG_DEBUG("fillActiveChannels done");
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("func=%s => epoll_wait timeout\n", __FUNCTION__);
    }
    else
    {
        if(savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("func=%s => epoll_wait error:%d\n", __FUNCTION__, savedErrno);
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activechannels) const
{
    LOG_DEBUG("fillActiveChannels numEvents:%u events_.size():%ld\n", numEvents, events_.size());
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for(int i = 0; i < numEvents; ++i)
    {   
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activechannels->push_back(channel);
    }
    LOG_DEBUG("fillActiveChannels done");
}