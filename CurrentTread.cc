#include "CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread 
{
    __thread int t_cachedtid = 0;
    
    void cacheTid()
    {
        if(t_cachedtid == 0)
        {
            t_cachedtid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}