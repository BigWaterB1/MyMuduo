#ifndef POLLER_H
#define POLLER_H

#include "nocopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

//myMuduo中多路事件分发器的IO复用模块
class Poller 
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default; //虚析构函数

    //接口函数,派生类实现具体的IO复用方法
    /*
     *一个类包含至少一个纯虚函数，这个类是抽象类
     *抽象类不能被实例化（即创建对象），因为至少有一个成员函数（即纯虚函数）没有被实现。
     */
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    //判断参数channel是否在poller管理的Channel列表中
    bool hasChannel(Channel* channel) const;

    //Eventloop可以通过该接口获取默认的IO复用的具体实现
    //返回基类指针
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    //用无序的哈希表效率会高一点,不需要排序了
    //map的key: sockfd, value: sockdf所属的Channel*通道类型
    //通过fd快速找到所属的Channel*
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
};
#endif