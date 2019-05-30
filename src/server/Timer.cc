#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <memory>
#include "Timer.h"

int64_t Timer::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// len >= 25
const char *Timer::timestr(int option, int64_t ms, char *buf, size_t len)
{
    struct tm tm;
    time_t seconds = ms / 1000;
    if (option == LOCAL_TIME)
        localtime_r(&seconds, &tm);
    else
        gmtime_r(&seconds, &tm);

    snprintf(buf, len, "%4d-%02d-%02d %02d:%02d:%02d.%04lld",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, ms % 1000);
    return buf;
}

void Timer::tick()
{
    if (!_timer.empty()) {
        const Timestamp *t = get();
        t->timerCb()();
        if (t->interval() > 0) {
            Timestamp *_t = new Timestamp;
            _t->setTimeout(t->interval());
            _t->setInterval(t->interval());
            _t->setTimerCb(t->timerCb());
            add(_t);
        }
        pop();
    }
}
