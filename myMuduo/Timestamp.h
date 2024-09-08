/*
获取当前时间
*/

#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include<iostream>
#include<string>
using std::string;

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpochArg);
    string toString() const;
    //get time of now
    static Timestamp now();//static修饰函数，可以直接调用，无需实例化对象
private:
    int64_t microSecondsSinceEpoch_;
};

#endif