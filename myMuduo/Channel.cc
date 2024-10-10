#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <poll.h>
#include <sys/epoll.h>


const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | POLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

//EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1) // 新创建的channel的index为-1，被Poller使用
    , logHup_(true)
    , tied_(false)
    , eventHandling_(false)
    , addedToLoop_(false)
{
}

Channel::~Channel()
{
}

//Channel的tie方法什么时候调用过?
//  tie_连接一个TcpConnection对象，观察TcpConnection是否被销毁
void Channel::tie(const std::shared_ptr<void> &obj)
{
  tie_ = obj;
  tied_ = true;
}

/*当改变channel所表示fd的events事件后，需要调用这个函数来向poller更新fd相应的事件epoll_ctl
 * EventLoop => ChannelList Poller
 */
void Channel::update()
{
  //通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
  loop_->updateChannel(this);
}

//在Channel所属的EventLoop中，调用这个函数来移除channel
void Channel::remove()
{
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  std::shared_ptr<void> guard;
  if(tied_)
  {
    guard = tie_.lock();//提升为强智能指针
    if(guard)//提升成功，说明TcpConnection对象还存在
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体的回调函数
// revents_是poller给channel设置好的
// 包括读事件、写事件、关闭事件、错误事件
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  LOG_DEBUG("Channel handle event:%d", revents_);
  if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
  {
    if(closeCallback_)
    {
      LOG_DEBUG("Channel::closeCallback_");
      closeCallback_();
    }
  }

  if(revents_ & EPOLLERR)
  {
    if(errorCallback_)
    {
      LOG_DEBUG("Channel::errorCallback_");
      errorCallback_();
    }
  }

  // 读事件
  if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLHUP))
  {
    if(readCallback_)
    {
      LOG_DEBUG("Channel::readCallback_");
      readCallback_(receiveTime);
    }
  }

  if(revents_ & EPOLLOUT)
  {
    if(writeCallback_)
    {
      LOG_DEBUG("Channel::writeCallback_");
      writeCallback_(); 
    }
  }

}