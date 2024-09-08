#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

namespace CurrentThread
{
    extern __thread int t_cachedtid; // __thread is a thread-local storage

    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedtid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedtid;
    }

}

#endif