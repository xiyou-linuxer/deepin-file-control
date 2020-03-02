#include <sys/epoll.h>
#include <vector>
#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"

int Epoll::wait(EventLoop *loop, int64_t timeout)
{
    int nevents = epoll_wait(_epfd, &_epfds[0], _epfds.capacity(), timeout);
    if (nevents > 0) {
        for (int i = 0; i < nevents; i++) {
            auto chl = loop->search(_epfds[i].data.fd);
            chl.get()->setRevents(_epfds[i].events);
            loop->fillActiveChannel(chl);
        }
    }
    return nevents;
}
