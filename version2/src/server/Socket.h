#ifndef _SOCKET_H
#define _SOCKET_H

#include <unistd.h>
#include "Noncopyable.h"

// 不可拷贝的
class Socket : Noncopyable {
public:
    Socket() {  }
    ~Socket() { close(_sockfd); }
    int fd() { return _sockfd; } 
    void setFd(int fd) { _sockfd = fd; }
    void setPort(int port) { _port = port; }
    void setNonblock();
    void setReuseAddr();
    void setNodelay();
    void listen();
    // void connect();
    int accept();
private:
    int _sockfd = -1;
    int _port = 0;
    static const int LISTENQ = 1024;
};

#endif // _SOCKET_H
