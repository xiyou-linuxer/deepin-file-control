#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <sys/poll.h>
#include <memory>
#include <functional>
#include "Buffer.h"
#include "Coder.h"
#include "Socket.h"
#include "Noncopyable.h"

class EventLoop;

// 不可拷贝的
class Channel : Noncopyable, 
    public std::enable_shared_from_this<Channel>  {
public:
    typedef std::function<void(std::shared_ptr<Channel>, Buffer&)> MessageCallback;
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCompleteCallback;
    enum {
        SENDING = 001, // 正在发送消息
    };
    explicit Channel(EventLoop *loop) : _loop(loop) {  }
    ~Channel() {  }
    int fd() { return _socket.fd(); }
    Socket& socket() { return _socket; }
    int events() { return _events; }
    void setRevents(int revents) { _revents = revents; }
    int isReading() { return _events & POLLIN; }
    int isWriting() { return _events & POLLOUT; }
    void enableRead() { _events |= POLLIN; }
    void enableWrite() { _events |= POLLOUT; changeEvent(); }
    void disableWrite() { _events &= ~POLLOUT; changeEvent(); }
    void changeEvent();
    void send(const char *s, size_t len);
    Request& req() { return _req; }
    void setStatus(int status_) { _status |= status_; }
    void clearStatus(int status_) { _status &= ~status_; }
    static const char *eventstr(int events);
    void setReadCb(const ReadCallback _cb) 
    { _readCb = _cb; }
    void setMessageCb(const MessageCallback _cb) 
    { _messageCb = _cb; }
    void setWriteCompleteCb(const WriteCompleteCallback _cb) 
    { _writeCompleteCb = _cb; }
    void handleEvent();
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void handleAccept();
private:
    EventLoop *_loop;
    Socket _socket;
    int _events = 0;
    int _revents = 0;
    int _status = 0;
    Buffer _input;
    Buffer _output;
    // better to use boost::any
    Request _req;
    ReadCallback _readCb = nullptr;
    MessageCallback _messageCb = nullptr;
    // 通常用于保证在写完消息后再关闭连接
    WriteCompleteCallback _writeCompleteCb = nullptr;
};

#endif // _CHANNEL_H
