#ifndef _CODER_H
#define _CODER_H

#include <string>

class Request {
public:
    enum {
        LINE        = 0x01,  // 请求行
        HEADER      = 0x02,  // 请求头部
        RECVING     = 0x04,  // 正在接收文件
        OK          = 0x08,  // 解析成功
        CONTINUE    = 0x10,
        ERROR       = 0x20,
        // SAVE        = 0x40,
        // GET         = 0x80,
    };
    int state() { return _state; }
    int fd() { return _fd; }
    size_t filesize() { return _filesize; }
    std::string& type() { return _type; }
    std::string& path() { return _path; }
    std::string& mac() { return _macAddr; }
    void setState(int state) { _state = state; }
    void setFd(int fd) { _fd = fd; }
    void setFilesize(size_t filesize) { _filesize = filesize; }
private:
    int _state = LINE;
    int _fd = -1;
    size_t _filesize = 0;
    std::string _type;
    std::string _path;
    std::string _macAddr;
};

#endif // _CODER_H
