#include "Buffer.h"

#include <sys/uio.h>
#include <sys/unistd.h>
/*
 * 从fd上读取数据，Poller工作在LT模式
 * Buffer缓冲区是有大小的，但是从fd上读取数据的时候，却不知道tcp数据最终的大小
 * 为了防止数据丢失，所以需要一个额外的缓冲区extrabuf
 * 如果从fd上读取的数据小于Buffer缓冲区的大小，则直接写入缓冲区
 * 如果从fd上读取的数据大于缓冲区的大小，则先将超出的部分写入extrabuf缓冲区，再扩容Buffer缓冲区
 * readv可以在非连续的内存中从同一个文件描述符fd写入数据
 */
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536]; // 栈上的内存空间 64k
    iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;     // 保存errno
    }
    else if (n <= writable) 
    {
        writeIndex_ += n;
    }
    else // n > writable extrabuf里面也写入了数据
    {
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable); //扩容
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    return n;
}