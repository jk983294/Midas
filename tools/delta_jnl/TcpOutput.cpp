#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "TcpOutput.h"

TcpOutput::TcpOutput(int fd, const string &addr_, long long bytes_, const timeval &now)
    : address(addr_), birth(now), totalBytesAtBirth(bytes_), totalBytes(bytes_) {
    int keepAlive = 1;
    socklen_t len = sizeof(keepAlive);
    int rc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, len);
    if (rc < 0) {
        cerr << "setsockopt(SO_KEEPALIVE) failed: " << strerror(errno) << "\n";
    }

    rc = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    if (rc < 0) {
        cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << "\n";
    }
}

/**
 * try best to insure all data is delivered before close
 * set SO_LINGER option, which is supposed to do this, this close may block for up to 5 seconds
 */
void TcpOutput::gentleClose(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);  // block IO
    struct linger l;
    l.l_onoff = 1;
    l.l_linger = 5;
    int rc = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
    if (rc < 0) {
        cerr << "setsockopt(SO_LINGER) failed: " << strerror(errno) << "\n";
    }
    close(fd);
}

bool TcpOutput::output(int fd, char *bufPtr, int bufLen, long long totalBytes_) {
    long count = totalBytes_ - totalBytes;
    long off = totalBytes % static_cast<long long>(bufLen);
    long end = off + count;

    if (end > bufLen) end = bufLen;  // wrap around

    long cc = write(fd, &bufPtr[off], end - off);

    if (cc < 0) {
        cerr << "TcpOutput write: " << strerror(errno) << "\n";
        return errno == EAGAIN;
    }

    if (cc == 0) {
        cerr << "TcpOutput write = 0\n";
        return false;
    }
    totalBytes += static_cast<long long>(cc);
    return true;
}

bool TcpOutput::input(int fd) const {
    char buf[2000];
    int cc = read(fd, buf, sizeof(buf));
    if (cc < 0) {
        cerr << "read " << address << " " << strerror(errno) << "\n";
        return errno == EAGAIN;
    }

    if (cc == 0) {
        cerr << "goodbye " << address << "\n";
        return false;
    }

    // extraneous blather
    cerr << "got " << cc << " bytes from " << address << "\n";
    for (int i = 0; i < cc; ++i) {
        unsigned char x = buf[i];
        unsigned int y = x;
        if (isgraph(x))
            cerr << x;
        else
            cerr << "(" << y << ")";
    }
    cerr << "\n";
    return true;
}
