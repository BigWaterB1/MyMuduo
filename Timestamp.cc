#include"Timestamp.h"

#include<time.h>

Timestamp::Timestamp()
    :microSecondsSinceEpoch_(0)
    {};

Timestamp::Timestamp(int64_t microSecondsSinceEpochArg)
    :microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {};

string Timestamp::toString() const
{
    char buf[128] = {0};
//   int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
//   int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
//   snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, sizeof(buf)-1,"%4d/%02d/%2d %02d:%02d:%02d", 
            tm_time->tm_year + 1900, 
            tm_time->tm_mon + 1,
            tm_time->tm_mday, 
            tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return buf;
};
//get time of now
Timestamp Timestamp::now()
{
//   struct timeval tv;
//   gettimeofday(&tv, NULL);
//   int64_t seconds = tv.tv_sec;
    return Timestamp(time(NULL));
};