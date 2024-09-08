#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

/*
 * epoll的使用
 * epoll_create() 创建epoll句柄
 * epoll_ctl() 注册事件到epoll句柄 add, mod, del
 * epoll_wait() 等待事件发生
 * epoll_destroy() 关闭epoll句柄
 */
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;//override让编译器去检查是否有覆盖

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override; 

private:
    static const int kInitEventListSize = 16;

    //填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activechannels) const;
    //更新channel通道 调用epoll_ctl
    void update(int operation, Channel* channel);

    int epollfd_;

    using EventList = std::vector<epoll_event>;
    EventList events_;
};

#endif