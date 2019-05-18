#include <iostream>
#include <memory>
#include <functional>
#include <signal.h>
#include "EventLoop.h"
#include "Channel.h"
#include "Buffer.h"
#include "Socket.h"
#include "Logger.h"
//#if defined (__linux__)
//#include "Epoll.h"
//#else
#include "Poll.h"
//#endif

void onPrint(std::shared_ptr<Channel> chl, Buffer& buf)
{
    std::cout << buf.c_str();
    buf.retrieveAll();
}

void printStr(const char *s)
{
    std::cout << s << std::endl;
}

void onMessage(std::shared_ptr<Channel> chl, Buffer& buf);

// 从外部中断(Ctr + C)和在logError()中调用exit()，都不会引发
// stack对象的析构，所以我们需要主动退出logger，从而保证将
// _flushBuf中的消息都写到文件中
static void sig_int(int signo)
{
    _log->quit();
    fprintf(stderr, "\n");
}

int main(void)
{
    signal(SIGINT,  sig_int);
    Logger logger;
    _log = &logger;
//#if defined (__linux__)
 //   Epoll epoll;
  //  EventLoop loop(&epoll);
//#else
    Poll poll;
    EventLoop loop(&poll);
//#endif
    Channel *chl = new Channel(&loop);
    chl->socket().setPort(8888);
    chl->socket().listen();
    logDebug("Server is listening port 8888");
    logDebug("listenfd is %d", chl->socket().fd());
    chl->setReadCb(std::bind(&Channel::handleAccept, chl));
    chl->setMessageCb(std::bind(&onMessage,
                std::placeholders::_1,
                std::placeholders::_2));
    loop.addChannel(chl);
    // e.g. 监听stdin
    // Channel *_in = new Channel(&loop);
    // _in->socket().setFd(0);
    // _in->setReadCb(std::bind(&Channel::handleRead, _in));
    // _in->setMessageCb(std::bind(&onPrint,
                // std::placeholders::_1,
                // std::placeholders::_2));
    // loop.addChannel(_in);
    // e.g. timer
    // loop.runEvery(200, std::bind(&printStr, "hello, world"));
    // loop.runEvery(800, std::bind(&printStr, "1234567"));
    loop.run();
}
