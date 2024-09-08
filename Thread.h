#ifndef THREAD_H
#define THREAD_H
/*
 * 对C++11的线程封装
 * thread -> Thread
 * 改进:chenshuo的muduo封装的是linux内核多线程
 */
#include "nocopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>

class Thread : nocopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const {return name_;}

    static int numCreated() { return numCreated_; }
private:
    void setDeafaultName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic<int> numCreated_;
};

#endif