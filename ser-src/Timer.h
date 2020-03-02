#ifndef _TIMER_H
#define _TIMER_H

#include <cstdlib>
#include <set>
#include <functional>
#include "Noncopyable.h"

typedef std::function<void()> TimerCallback;

class Timestamp {
public:
    int64_t timeout() const { return _timeout; } 
    int64_t interval() const { return _interval; }
    TimerCallback timerCb() const { return _timerCb; }
    void setTimeout(int64_t timeout) { _timeout = timeout; }
    void setInterval(int64_t interval) { _interval = interval; }
    void setTimerCb(TimerCallback _cb) { _timerCb = _cb; }
private:
    int64_t _timeout = 0;
    int64_t _interval = 0;
    TimerCallback _timerCb;
};

class TimestampCmp {
public:
    bool operator()(const std::unique_ptr<Timestamp>& lhs,
                    const std::unique_ptr<Timestamp>& rhs) const
    {
        return lhs.get()->timeout() < rhs.get()->timeout();
    }
};

// 暂时不支持取消定时器
class Timer : Noncopyable {
public:
    enum TIMESTR_STATE {
        GMT_TIME,    // GMT时间
        LOCAL_TIME,  // 本地时间
    };
    static int64_t now();
    static const char *timestr(int option, int64_t ms, char *buf, 
            size_t len);
    // 添加一个定时器
    void add(Timestamp *_t)
    {
        _t->setTimeout(_t->timeout() + now());
        _timer.insert(std::unique_ptr<Timestamp>(_t));
    }
    // 取出最小定时器
    const Timestamp *get() { return _timer.cbegin()->get(); }
    // 弹出最小定时器事件
    void pop() { _timer.erase(_timer.begin()); }
    // 返回最小超时值
    int64_t timeout()
    {
        int64_t _timeval;
        if (!_timer.empty())
            return (_timeval = llabs(get()->timeout() - now())) > 0 
                    ? _timeval : 1;
        else
            return -1;
    }
    // 处理所有到期的定时事件
    void tick();
private:
    std::set<std::unique_ptr<Timestamp>, TimestampCmp> _timer;
};

#endif // _TIMER_H
