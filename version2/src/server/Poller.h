#ifndef _POLLER_H
#define _POLLER_H

class EventLoop;

class Poller {
public:
    virtual ~Poller() {  }
    virtual int wait(EventLoop *loop, int64_t timeout) = 0;
    virtual void add(int fd, int events) = 0;
    virtual void change(int fd, int events) = 0;
    virtual void del(int fd) = 0;
};

#endif // _POLLER_H
