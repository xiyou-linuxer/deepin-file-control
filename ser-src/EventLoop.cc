#include <cstring>
#include <sys/socket.h>
#include <errno.h>
#include <functional>
#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "Timer.h"

void EventLoop::run(void)
{
    // 每隔 1s 将Log Buffer中的数据flush到文件中
    // runEvery(1000 * 1, std::bind(&Logger::wakeUp, _log));
    // 5s后退出loop
    // runAfter(1000 * 5, std::bind(&EventLoop::quit, this));

    while (!_quit) {
        int timeout = _timer.timeout();
        logDebug("timeout is %d", _timer.timeout());
        int nevents = _poller->wait(this, timeout);
        logDebug("there are %d active events", nevents);
        _loop = 1;
        if (nevents > 0) {
            for (auto& it : _activeChannels)
                it.get()->handleEvent();
            _activeChannels.clear();
        } else if (nevents == 0)
            _timer.tick();
        else {
            if (errno != EINTR)
                logWarn("poller->wait error: %s", strerror(errno));
        }
        _loop = 0;
    }
}

void EventLoop::runAfter(int64_t timeout, TimerCallback _cb)
{
    Timestamp *t = new Timestamp;
    t->setTimeout(timeout);
    t->setInterval(0);
    t->setTimerCb(_cb);
    _timer.add(t);
}

void EventLoop::runEvery(int64_t interval, TimerCallback _cb)
{
    Timestamp *t = new Timestamp;
    t->setTimeout(interval);
    t->setInterval(interval);
    t->setTimerCb(_cb);
    _timer.add(t);
}
