#ifndef _EPOLL_H
#define _EPOLL_H

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include "Poller.h"
#include "Noncopyable.h"

class EventLoop;

class Epoll : public Poller, Noncopyable {
public:
    Epoll() : _nfds(0) { _epfd = epoll_create(1); }
    ~Epoll() { close(_epfd); }
    int wait(EventLoop *loop, int64_t timeout);
    void add(int fd, int events)
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) < 0)
            ;
        _epfds.resize(++_nfds);
    }
    void change(int fd, int events)
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
            ;
    }
    void del(int fd)
    {
        if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) < 0)
            ;
    }
private:
    std::vector<struct epoll_event> _epfds;
    int _epfd;
    int _nfds;
};

#endif // _EPOLL_H
