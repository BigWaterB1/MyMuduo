#include "Poller.h"
#include "Channel.h"//使用了Channel的方法

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());
    // if(it != channels_.end()) return true;
    // else return false;
    return it != channels_.end() && it->second == channel;
}