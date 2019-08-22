#ifndef _EVENTLOOP_H
#define _EVENTLOOP_H

#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include "Poller.h"
#include "Channel.h"
#include "Timer.h"
#include "Noncopyable.h"
#include "Logger.h"

// 不可拷贝的
class EventLoop : Noncopyable {
public:
    explicit EventLoop(Poller *poller) 
        : _poller(poller), _quit(0), _loop(0)
    {  }
    ~EventLoop() {  }
    // 向loop中添加一个新的Channel
    void addChannel(Channel *chl)
    {
        chl->enableRead();
        _poller->add(chl->fd(), chl->events());
        _channelMap.insert(std::pair<int,
                std::shared_ptr<Channel>>(chl->fd(), chl));
    }
    // 从loop中移除一个Channel
    void delChannel(Channel *chl)
    {
        logDebug("fd(%d) is closed", chl->fd());
        _poller->del(chl->fd());
        _channelMap.erase(chl->fd());
    }
    void changeEvent(int fd, int events) 
    { 
        _poller->change(fd, events);
    }
    std::shared_ptr<Channel> search(int fd)
    {
        auto it = _channelMap.find(fd);
        assert(it != _channelMap.end());
        logDebug("found <fd(%d), chl>", fd);
        return it->second;
    }
    void fillActiveChannel(std::shared_ptr<Channel> chl) 
    { 
        logDebug("chl(fd(%d)) is added to _activeChannels", chl->fd());
        _activeChannels.push_back(chl); 
    }
    void runAfter(int64_t timeout, TimerCallback _cb);
    void runEvery(int64_t interval, TimerCallback _cb);
    void run();
    void quit() { _quit = 1; }
private:
    Poller *_poller;
    std::map<int, std::shared_ptr<Channel>> _channelMap;
    std::vector<std::shared_ptr<Channel>> _activeChannels;
    Timer _timer;
    int _quit;
    int _loop;
};

#endif // _EVENTLOOP_H
