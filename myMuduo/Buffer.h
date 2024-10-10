#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>
#include <string.h>

//buffer 模型
/*
    +-----------------------------+-------------------------+--------------------------+
    |  --- prependable Bytes ---  |  --- readableBytes ---  |  --- writableBytes() --- |
            (kCheapPrepend)
    +-----------------------------+-------------------------+--------------------------+
    |                             |                         |                          |
    0                         readIndex_               writeIndex_               buffer_.size()

    1. 读数据
       全部读取完毕  -> 复位 readIndex_ = writeIndex_ = kCheapPrepend
       部分读取完毕  -> readIndex_ += len
    2. 写数据
       可写空间不足  -> 扩容
       (writableBytes() + prependableBytes() < len + kCheapPrepend)  
       可写空间足够  -> 直接写数据到buffer_
*/
class Buffer 
{
public: 
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t ininialSize =  kInitialSize)
        : buffer_(kCheapPrepend + ininialSize)
        , readIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readIndex_;
    }

    // for debug
    std::string toString() const
    {
        return std::string(peek(), static_cast<int>(readableBytes()));
    }

    /*接下来的用于http 数据解析
     * CRLF意为回车换行
     * 回车(CR，ASCII码为13, \\r) 换行(LF, ASCII码为10, \\n)
    */ 

    const char *findCRLF() const
    {
        /** 可以改成 memmem 函数
         * void *memmem(const void *haystack, size_t haystacklen,
         *              const void *needle, size_t needlelen);
         * 功能 : 在一块内存中寻找匹配另一块内存的内容的第一个位置
         * 返回值 : 返回该位置的指针，如找不到，返回空指针。
         */
        const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char *findCRLF(const char *start) const
    {
        // FIXME: replace with memmem()?
        const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char *findEOL() const
    {
        const void *eol = memchr(peek(), '\n', readableBytes());
        return static_cast<const char *>(eol);
    }

    const char *findEOL(const char *start) const
    {
        const void *eol = memchr(start, '\n', beginWrite() - start);
        return static_cast<const char *>(eol);
    }
    /* 以上为http 解析函数 */

    // 恢复
    void retrieve(size_t len) 
    {
        if(len < readableBytes()) // 没有读完数据
        {
            readIndex_ += len;
        }
        else // 读完数据
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char *end)
    {
        retrieve(end - peek());
    }

    void retrieveAll()
    {
        readIndex_ = writeIndex_ = kCheapPrepend;
    }

    // 把oMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 读取完数据后，复位
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    // 可以接收string 和 const char* ， 自动给出长度
    void append(const std::string& str)
    {
        append(str.data(), str.size());
    }

    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writeIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writeIndex_;
    }

    // 从fd读取数据到buffer中
    ssize_t readFd(int fd, int* savedErrno);

    ssize_t writeFd(int fd, int* savedErrno);

private:
    //返回数组首元素地址的裸指针
    char* begin() 
    { 
        // &(it.operator*())
        return &*buffer_.begin(); 
    }

    const char* begin() const
    { 
        return &*buffer_.begin(); 
    }

    // 扩容
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writeIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readIndex_,
                      begin() + writeIndex_,
                      begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = kCheapPrepend + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;

    static const char kCRLF[];
};

#endif