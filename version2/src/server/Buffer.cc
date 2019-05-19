#include "Buffer.h"

const char Buffer::_crlf[] = "\r\n";

// 将fd中的数据读到Buffer中
int Buffer::readFd(int fd)
{
    char extrabuf[65536];
    struct iovec iov[2];
    size_t writen = writeable();
    ssize_t n;

    iov[0].iov_base = begin() + _writeindex;
    iov[0].iov_len = writen;
    iov[1].iov_base = extrabuf;
    iov[1].iov_len = sizeof(extrabuf);

    if ((n = readv(fd, iov, 2)) > 0) {
        if (n <= writen)
            _writeindex += n;
        else {
            _writeindex += writen;
            append(extrabuf, n - writen);
        }
    }
    return n;
}
