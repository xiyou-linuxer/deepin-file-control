#ifndef _POLL_H
#define _POLL_H

#include <cassert>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include "Noncopyable.h"
#include "Poller.h"

class EventLoop;

// 不可拷贝的
class Poll : public Poller, Noncopyable {
public:
    // 事件多路分发器
    int wait(EventLoop *loop, int64_t timeout);
    // 注册一个新的fd
    void add(int fd, int events)
    {
        struct pollfd ev;
        ev.fd = fd;
        ev.events = events;
        ev.revents = 0;
        _pollfds.push_back(ev);
        _indexs.insert(std::pair<int, int>(ev.fd, _pollfds.size() - 1));
    }
    // 修改fd上的事件
    void change(int fd, int events)
    {
        auto it = _indexs.find(fd);
        assert(it != _indexs.end());
        struct pollfd *pfd = &_pollfds[it->second];
        pfd->events = events;
    }
    // 从内核关注列表中移除fd
    void del(int fd)
    {
        auto it = _indexs.find(fd);
        assert(it != _indexs.end());
        size_t end = _pollfds.size() - 1;
        std::swap(_pollfds[it->second], _pollfds[end]);
        _pollfds.pop_back();
        _indexs.erase(fd);
    }
private:
    std::vector<struct pollfd> _pollfds;
    // <fd, index>, index for _pollfds
    std::map<int, int> _indexs;
};

#endif // _POLL_H
