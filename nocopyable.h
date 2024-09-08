#ifndef NOCOPYABLE_H
#define NOCOPYABLE_H

/*
 * nocopyable被派生类继承后，
 * 派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
 */
class nocopyable
{
public:
    nocopyable(const nocopyable& src) = delete;
    nocopyable& operator=(const nocopyable& src) =delete;

protected:
    nocopyable() = default;
    ~nocopyable() = default; 
};

#endif
