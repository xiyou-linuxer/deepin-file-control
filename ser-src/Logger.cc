#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <chrono>
#include "EventLoop.h"
#include "Timer.h"
#include "Logger.h"

Logger *_log = nullptr;

Logger::Logger()
    : _thread([this]{ this->flushToFile(); }),
    _quit(0)
{
    if (_log)
        logError("repeat creat logger");
    mkdir(".log", 0777);
    // 为了方便测试查看，所以暂时只使用一个文件
    _fd = open("./.log/x.log", O_WRONLY | O_APPEND | O_CREAT, 0777);
}

Logger::~Logger()
{
    _quit = 1;
    _condVar.notify_one();
    _thread.join();
    close(_fd);
}

void Logger::writeToBuffer(const char *s, size_t len)
{
    std::lock_guard<std::mutex> mlock(_mutex);
    _writeBuf.append(s, len);
}

// void Logger::wakeUp(void)
// {
    // std::lock_guard<std::mutex> mlock(_mutex);
    // if (_writeBuf.readable() > 0) {
        // _writeBuf.swap(_flushBuf);
        // _condVar.notify_one();
    // }
// }

void Logger::flushToFile()
{
    while (1) {
        waitFor();
        if (_flushBuf.readable() > 0)
            writeToFile();
    }
}

void Logger::waitFor()
{
    std::unique_lock<std::mutex> mlock(_mutex);
    if (!_quit && _writeBuf.readable() == 0)
        _condVar.wait_for(mlock, std::chrono::seconds(1));
    if (_quit) {
        write(_fd, _flushBuf.peek(), _flushBuf.readable());
        exit(1);
    }
    if (_writeBuf.readable() > 0)
        _writeBuf.swap(_flushBuf);
}

void Logger::writeToFile()
{
    // 将日志打印到屏幕上
    // fprintf(stderr, "%s", _flushBuf.c_str());
    // _flushBuf.retrieveAll();
    while (1) {
        ssize_t n = write(_fd, _flushBuf.peek(), _flushBuf.readable());
        if (n < 0) {
            fprintf(stderr, "flushToFile write error: %s", strerror(errno));
            exit(1);
        }
        _flushBuf.retrieve(n);
        if (_flushBuf.readable() == 0)
            break;
    }
}

void Logger::output(int level, const char *file, int line,
        const char *func, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    std::string str;
    char buf[65536];
    str += levelStr(level);
    if (level == DEBUG)
        str += ": ";
    else if (level == INFO)
        str += ":  ";
    else if (level == WARN)
        str += ":  ";
    else if (level == ERROR)
        str += ": ";
    str += Timer::timestr(Timer::LOCAL_TIME, Timer::now(),
            buf, sizeof(buf));
    str += " ";
    str += file;
    str += ":";
    snprintf(buf, sizeof(buf), "%d", line);
    str += buf;
    str += ":";
    str += func;
    str += ": [";
    vsnprintf(buf, sizeof(buf), fmt, ap);
    str += buf;
    str += "]\n";
    writeToBuffer(str.c_str(), str.size());
    va_end(ap);
    if (level == ERROR) {
        fprintf(stderr, ">>logError\n");
        quit();
    }
}

const char *Logger::levelStr(int level)
{
    if (level == DEBUG)
        return "DEBUG";
    else if (level == INFO)
        return "INFO";
    else if (level == WARN)
        return "WARN";
    else if (level == ERROR)
        return "ERROR";
    else
        return "NONE";
}
