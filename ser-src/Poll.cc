#include <sys/poll.h>
#include <vector>
#include "Poll.h"
#include "EventLoop.h"
#include "Channel.h"

int Poll::wait(EventLoop *loop, int64_t timeout)
{
    int nevents = poll(&_pollfds[0], _pollfds.size(), timeout);
    int ret = nevents;
    if (nevents > 0) {
        for (int i = 0; i < _pollfds.size() && nevents > 0; i++) {
            if (_pollfds[i].revents > 0) {
                auto chl = loop->search(_pollfds[i].fd);
                chl.get()->setRevents(_pollfds[i].revents);
                loop->fillActiveChannel(chl);
                nevents--;
            }
        }
    }
    return ret;
}
