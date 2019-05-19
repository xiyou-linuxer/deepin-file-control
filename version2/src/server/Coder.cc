#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <memory>
#include "Channel.h"
#include "Buffer.h"
#include "Coder.h"
#include "Logger.h"

void fileMkdir(const char *pathname, const char *mac)
{
    std::string dir("./backupfile/");
	dir += mac;
	dir += "/";
    dir += pathname;

    auto tr = dir.begin() + 2;
    while (tr <= dir.end()) {
        for ( ; tr != dir.end(); tr++) {
            if(*tr == '/')
                break;
        }
        if (tr >= dir.end())
            break;
        std::string s;
        s.insert(s.begin(), dir.begin(), tr);
        mkdir(s.c_str(), 0777);
        tr++;
    }
}

void parseLine(Request& req, Buffer& buf, int len)
{
    char *p = buf.peek();
    char *ep = p + len;

    logInfo("parsing request line");
    req.type().clear();
    req.path().clear();
    req.mac().clear();
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    while (p < ep && !isspace(*p))
        req.type().push_back(*p++);
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    while (p < ep && !isspace(*p))
        req.path().push_back(*p++);
    while (p < ep && (*p == ' ' || *p == '\t'))
        p++;
    while (p < ep && !isspace(*p))
        req.mac().push_back(*p++);
    req.setState(Request::HEADER);
}

void parseHeader(Request& req, Buffer& buf, int len)
{
    char *p = buf.peek();
    char *ep = p + len;

    logInfo("parsing request header");
    // 收到了空行
    if (p == ep) {
        if (req.type() == "SAVE") {
            logInfo("recv SAVE request");
            req.setState(Request::OK);
        } else if (req.type() == "GET") {
            logInfo("recv GET request");
            req.setState(Request::OK);
        }
        return;
    }
    if (strncasecmp(p, "filesize:", 9) == 0) {
        p += 9;
        while (p < ep && (*p == ' ' || *p == '\t'))
            p++;
        req.setFilesize(atoi(p));
        logInfo("recv [filesize] field");
    } else {
        ; // 目前头部只有一个字段
    }
}

// 解析客户请求
void parseRequest(Buffer& buf, Request& req)
{
    // 可能有一行完整的消息
    if (buf.readable() < 2)
        logInfo("recv %d bytes, less than 2(\r\n) chars", buf.readable());
    while (buf.readable() >= 2) {
        int crlf = buf.findCrlf();
        // 至少有一行请求
        if (crlf >= 0) {
            logInfo("recv %d bytes, is one line");
            logInfo("enter parse, state is %s", req.stateStr().c_str());
            if (req.state() == Request::LINE) {
                parseLine(req, buf, crlf);
                crlf += 2;
            } else if (req.state() == Request::HEADER) {
                parseHeader(req, buf, crlf);
                crlf += 2;
            } else
                break;
            buf.retrieve(crlf);
        } else {
            logInfo("recv %d bytes, less than one line", buf.readable());
            break;
        }
    }
}

std::string getPathname(Request& req)
{
    std::string pathname("./backupfile/");
    pathname += req.mac() + req.path();
    return pathname;
}

// SAVE-OK\r\n
// filesize: 0\r\n
// \r\n
void replySaveOk(Channel *chl, Request& req)
{
    std::string s("SAVE-STATUS ");
    s += req.path();
    s += "\r\n";
    s += "filesize: 0\r\n";
    s += "\r\n";
    chl->send(s.c_str(), s.size());
    logDebug("send %d bytes to fd(%d): %s", s.size(),
            chl->fd(), s.c_str());
}

// GET-OK path\r\n
// filesize: size\r\n
// \r\n
// Text
void replyGetOk(Channel *chl, Request& req)
{
    char buf[32];
    struct stat st;
    std::string pathname = getPathname(req);
    if (lstat(pathname.c_str(), &st) < 0) {
        logWarn("lstat file(%s) error: %s", pathname.c_str(),
                strerror(errno));
        req.setState(Request::ERROR);
        return;
    }
    snprintf(buf, sizeof(buf), "%lld\r\n", st.st_size);
    std::string s("GET-STATUS ");
    s += req.path();
    s += "\r\n";
    s += "filesize: ";
    s += buf;
    s += "\r\n";
    chl->send(s.c_str(), s.size());
    logDebug("send %d bytes to fd(%d): %s", s.size(),
            chl->fd(), s.c_str());
}

// 接收客户端发来的文件以更新本地文件
void recvFile(Channel *chl, Buffer& buf, Request& req)
{
    std::string pathname = getPathname(req);
    logInfo("recving file from fd(%d)", chl->fd());
    if (req.fd() < 0) {
        int fd = open(pathname.c_str(), O_WRONLY | O_APPEND
                | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            logWarn("file(%s) can't open: %s", pathname.c_str(),
                    strerror(errno));
            req.setState(Request::ERROR);
            return;
        }
        req.setFd(fd);
    }
    size_t readable = buf.readable();
    logInfo("readable is %zu bytes, filesize is %zu bytes", readable,
            req.filesize());
    if (readable >= req.filesize()) {
        readable = req.filesize();
        req.setState(Request::LINE);
    } else { // 文件还未接收完
        req.setFilesize(req.filesize() - readable);
        req.setState(req.state() | Request::RECVING);
    }
    while (1) {
        ssize_t n = write(req.fd(), buf.peek(), readable);
        if (n < 0) {
            logWarn("can't write to file(%s): %s", pathname.c_str(),
                    strerror(errno));
            req.setState(Request::ERROR);
            return;
        }
        logDebug("write %zu bytes to file(%s)", n, pathname.c_str());
        buf.retrieve(n);
        if (n < readable)
            readable -= n;
        else
            break;
    }
    if (req.state() == Request::LINE) {
        logInfo("file recved, sending SAVE response");
        replySaveOk(chl, req);
        close(req.fd());
        req.setFd(-1);
        if (buf.readable() > 0)
            req.setState(req.state() | Request::CONTINUE);
    }
}

// 将备份文件发送给客户
void sendFile(Channel *chl, Request& req)
{
    Buffer buf;
    std::string pathname = getPathname(req);
    logInfo("sending file");
    int fd = open(pathname.c_str(), O_RDONLY);
    if (fd < 0) {
        logWarn("file(%s) can't open: %s", pathname.c_str(),
                strerror(errno));
        req.setState(Request::ERROR);
        return;
    }
    while (buf.readFd(fd) > 0) {
        chl->send(buf.peek(), buf.readable());
        buf.retrieveAll();
    }
    close(fd);
    req.setState(Request::LINE);
}

// 向客户回复响应信息
void replyResponse(Channel *chl, Buffer& buf, Request& req)
{
    if (req.type() == "SAVE") {
        recvFile(chl, buf, req);
    } else if (req.type() == "GET") {
        logInfo("sending GET response");
        replyGetOk(chl, req);
        if (req.state() != Request::ERROR)
            sendFile(chl, req);
    }
}

void onMessage(std::shared_ptr<Channel> chl, Buffer& buf)
{
    // std::cout << ">>client: " << std::endl << buf.c_str();
    // buf.retrieveAll();
    Channel *chlptr = chl.get();
    Request& req = chl->req();
    logInfo("start parsing, state is %s", req.stateStr().c_str());
    if (req.state() & Request::RECVING) {
        // 继续接收剩余的文件
        recvFile(chlptr, buf, req);
        return;
    } else {
        // 解析新到来的请求
_continue:
        parseRequest(buf, req);
        logInfo("parsed, state is %s", req.stateStr().c_str());
    }
    if (req.state() & Request::OK) {
        // printInfo(req);
        logDebug("recv request from fd(%d): [type: %s] [path: %s] [mac: %s] [filesize: %zu]",
                chl->fd(), req.type().c_str(), req.path().c_str(),
                req.mac().c_str(), req.filesize());
        fileMkdir(req.path().c_str(), req.mac().c_str());
        logInfo("parse successfully, constructing response");
        replyResponse(chlptr, buf, req);
        logInfo("sended response, state is %s", req.stateStr().c_str());
        if (req.state() == Request::ERROR) {
            chl->handleClose();
            return;
        } else if (req.state() & Request::CONTINUE) {
            req.setState(req.state() & ~Request::CONTINUE);
            goto _continue;
        }
    }
}

std::string Request::stateStr(void)
{
    std::string s;
    if (state() & LINE)
        s += "LINE|";
    if (state() & HEADER)
        s += "HEADER|";
    if (state() & RECVING)
        s += "RECVING|";
    if (state() & OK)
        s += "OK|";
    if (state() & CONTINUE)
        s += "CONTINUE|";
    if (state() & ERROR)
        s += "ERROR|";
    return s;
}
