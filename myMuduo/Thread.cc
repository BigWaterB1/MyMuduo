#include "Thread.h"
#include "Logger.h"
#include "CurrentThread.h"

#include <semaphore.h>
#include <iostream>

std::atomic<int> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false),
      joined_(false),
      tid_(0),
      func_(std::move(func)),
      name_(name)
{
    setDeafaultName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach();
    }
}

// 真正开启线程
void Thread::start()
{
    sem_t sem;
    sem_init(&sem, 0, 0);
    //pthread_create(&tid_, nullptr, func_, this);
    started_ = true;
    thread_ = std::shared_ptr<std::thread>(new std::thread(
        [&]() 
        {
            tid_ = CurrentThread::tid();
            sem_post(&sem);
            func_();
        }
    ));

    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDeafaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}