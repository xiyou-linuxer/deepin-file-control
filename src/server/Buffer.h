#ifndef _ANGEL_BUFFER_H
#define _ANGEL_BUFFER_H

#include <vector>
#include <algorithm>

class Buffer {
public:
    Buffer() : _buf(INIT_SIZE) {  }
    ~Buffer() {  }
    static const size_t INIT_SIZE = 1024;
    char *begin() { return &*_buf.begin(); }
    char *peek() { return begin() + _readindex; }
    size_t prependable() const { return _readindex; }
    size_t readable() const { return _writeindex - _readindex; }
    size_t writeable() const { return _buf.size() - _writeindex; }
    void append(const char *data, size_t len)
    {
        makeSpace(len);
        std::copy(data, data + len, _buf.begin() + _writeindex);
        _writeindex += len;
    }
    // 内部腾挪
    void makeSpace(size_t len)
    {
        if (len > writeable()) {
            if (len <= writeable() + prependable()) {
                size_t readBytes = readable();
                std::copy(peek(), peek() + readBytes, begin());
                _readindex = 0;
                _writeindex = _readindex + readBytes;
            } else
                _buf.resize(_writeindex + len);
        }
    }
    // 返回C风格字符串
    const char *c_str() 
    { 
        makeSpace(1);
        _buf[_writeindex] = '\0';
        return peek();
    }
    int findStr(char *s, const char *p, size_t plen)
    {
        const char *pattern = std::search(s, begin() + _writeindex, p, p + plen);
        return pattern == begin() + _writeindex ? -1 : pattern - s;
    }
    int findStr(const char *s, size_t len)
    {
        return findStr(peek(), s, len);
    }
    // 返回\r\n在Buffer中第一次出现的位置，没出现返回-1
    int findCrlf() { return findStr("\r\n", 2); }
    int findLf() { return findStr("\n", 1); }
    // 跳过已读的数据
    void retrieve(size_t len)
    {
        if (len < readable())
            _readindex += len;
        else
            _readindex = _writeindex = 0;
    }
    void retrieveAll() { retrieve(readable()); }
    int readFd(int fd);
    void swap(Buffer& _buffer)
    {
        _buf.swap(_buffer._buf);
        std::swap(_readindex, _buffer._readindex);
        std::swap(_writeindex, _buffer._writeindex);
    }
    char& operator[](size_t idx) { return _buf[idx]; }
private:
    std::vector<char> _buf;
    size_t _readindex = 0;
    size_t _writeindex = 0;
};

#endif // _ANGEL_BUFFER_H
