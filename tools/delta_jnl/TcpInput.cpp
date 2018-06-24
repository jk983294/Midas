#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "TcpInput.h"

TcpInput::TcpInput(int fd, const string &host_, const string &service_, const string &iface, bool noDelay_,
                   const timeval &now)
    : host(host_),
      port(service_),
      beginBuffer(new char[bufferLen()]),
      endBuffer(&beginBuffer[bufferLen()]),
      warningTrack(endBuffer - maxBlockLen()),
      beginData(beginBuffer),
      endData(beginBuffer),
      birth(now) {
    if (!iface.empty())
        if (!setInterface(fd, iface)) return;

    if (!setNoDelay(fd, noDelay_)) return;

    msgsThen[0] = msgsThen[1] = 0;
    usecondsThen[0] = usecondsThen[1] = 0;

    then[0].tv_sec = then[1].tv_sec = now.tv_sec;
    then[0].tv_usec = then[1].tv_usec = now.tv_usec;

    if (setBlockingIo(fd, false))
        if (setKeepAlive(fd)) becomeClient(fd);
}

TcpInput::~TcpInput() { delete[] beginBuffer; }

void TcpInput::occasional(const timeval &now) {
    static const long long N = 10000000;
    long long u = useconds - usecondsThen[1];
    long s = now.tv_sec - then[1].tv_sec;
    if (u <= N || s <= 20) {
        if (s <= 120) return;
    }

    then[0].tv_sec = then[1].tv_sec;
    then[0].tv_usec = then[1].tv_usec;
    then[1].tv_sec = now.tv_sec;
    then[1].tv_usec = now.tv_usec;
    usecondsThen[0] = usecondsThen[1];
    usecondsThen[1] = useconds;
    msgsThen[0] = msgsThen[1];
    msgsThen[1] = msgs;
}

void TcpInput::serviceRead(int fd) {
    if (beginData > warningTrack) {
        // no room in buffer for biggest message
        int dataBytes = endData - beginData;
        memcpy(beginBuffer, beginData, dataBytes);
        beginData = beginBuffer;
        endData = &beginBuffer[dataBytes];
    }

    if (endData == endBuffer) return;

    int cc = read(fd, endData, endBuffer - endData);
    if (cc > 0) {
        endData += cc;
        return;
    }

    if (cc < 0 && errno == EAGAIN) return;

    if (cc)
        cerr << "read fails: " << strerror(errno) << "\n";
    else
        cerr << "connection lost (EOF)\n";

    state = Broken;
}

void TcpInput::connectDone(int fd) {
    if (state != Trying) cerr << "logic error connectDone\n";

    int connectErrno;
    socklen_t len = sizeof(connectErrno);
    int rc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &connectErrno, &len);
    if (rc < 0) {
        cerr << "getsockopt(SO_ERROR) failed: " << strerror(errno) << "\n";
        state = Broken;
        return;
    }

    if (connectErrno) {
        cerr << "connect fails: " << strerror(connectErrno) << "\n";
        state = Broken;
        return;
    }

    cout << "server " << host << ":" << port << " active...\n";
    state = Active;
}

int TcpInput::socket_fd() { return socket(AF_INET, SOCK_STREAM, 0); }

void TcpInput::becomeClient(int fd) {
    unsigned short portNum = std::atoi(port.c_str());
    if (!portNum) {
        servent *s(getservbyname(port.c_str(), "tcp"));
        if (!s) {
            cerr << "getservbyname() failed: " << strerror(errno) << "\n";
            state = Broken;
            return;
        }
        portNum = ntohs(s->s_port);
    }

    hostent *hE = gethostbyname(host.c_str());
    if (!hE) {
        cerr << "gethostbyname() failed: " << strerror(errno) << "\n";
        state = Broken;
        return;
    }

    struct sockaddr_in inAddr;
    memset(&inAddr, 0, sizeof(inAddr));
    inAddr.sin_family = AF_INET;
    inAddr.sin_port = htons(portNum);
    memcpy(&inAddr.sin_addr.s_addr, hE->h_addr_list[0], hE->h_length);

    if (connect(fd, (sockaddr *)&inAddr, sizeof(inAddr)) < 0) {
        if (errno == EINPROGRESS) {
            cerr << "server " << host << ":" << port << " trying...\n";
            state = Trying;
        } else {
            cerr << "connect() failed: " << strerror(errno) << "\n";
            state = Broken;
        }
    } else {
        state = Active;
    }
}

bool TcpInput::setBlockingIo(int fd, bool yes) {
    int long mask(fcntl(fd, F_GETFL, 0));
    if (yes)
        mask &= ~O_NONBLOCK;
    else
        mask |= O_NONBLOCK;

    int rc = fcntl(fd, F_SETFL, mask);
    if (rc < 0) {
        cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool TcpInput::setKeepAlive(int fd) {
    int val = 1;
    socklen_t len = sizeof(val);
    int rc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, len);
    if (rc < 0) {
        cerr << "setsockopt(SO_KEEPALIVE) failed: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool TcpInput::setInterface(int fd, const string &iface) {
    hostent *hE = gethostbyname(iface.c_str());
    if (!hE) {
        cerr << "setInterface gethostbyname() failed: " << iface << "\n";
        return false;
    }

    struct sockaddr_in inAddr;
    memset(&inAddr, 0, sizeof(inAddr));
    inAddr.sin_family = AF_INET;
    memcpy(&inAddr.sin_addr.s_addr, hE->h_addr_list[0], hE->h_length);
    unsigned char x[4];
    memcpy(x, hE->h_addr_list[0], hE->h_length);

    int rc = bind(fd, (struct sockaddr *)&inAddr, sizeof(inAddr));
    if (rc < 0) {
        cerr << "bind() failed: " << strerror(errno) << "\n";
        return false;
    }

    cout << "bound to " << int(x[0]) << "." << int(x[1]) << "." << int(x[2]) << "." << int(x[3]) << "\n";
    return true;
}

bool TcpInput::setNoDelay(int fd, bool yes) {
    int n = yes ? 1 : 0;
    int rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof(n));
    if (rc < 0) {
        cerr << "setsockopt(TCP_NODELAY) failed: " << strerror(errno) << "\n";
        return false;
    }

    int m = yes ? 0 : 1;
    socklen_t len = sizeof(m);
    rc = getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &m, &len);
    if (rc < 0) {
        cerr << "getsockopt(TCP_NODELAY) failed: " << strerror(errno) << "\n";
        return false;
    }

    if (m != n) {
        cerr << "upstream TCP NODELAY failed\n";
        return false;
    }

    return true;
}

DeltaPass::DeltaPass(int fd, const string &host_, const string &service_, const string &iface, bool noDelay_,
                     const timeval &now)
    : TcpInput(fd, host_, service_, iface, noDelay_, now) {}

int DeltaPass::getBlock(char *p, const timeval &now) {
    const long n = endData - beginData;
    const char *const s = beginData;

    if (n < 16) return 0;  // incomplete header

    uint16_t rawCount;
    memcpy(&rawCount, &s[0], 2);
    const int len = 2 + ntohs(rawCount);
    if (n < len) return 0;  // incomplete message

    beginData += len;

    if (len > maxBlockLen()) {
        cerr << "message too large!\n";
        return 0;
    }

    uint8_t flag;
    memcpy(&flag, &s[10], 1);
    if (flag != 0xFF) {
        cerr << "missing magic number\n";
        return -1;
    }

    uint32_t rawSecond;
    memcpy(&rawSecond, &s[2], 4);
    const long second = ntohl(rawSecond);

    memcpy(p, s, len);
    static const long long M = 1000000;
    useconds += static_cast<long long>(now.tv_sec - second) * M;
    bytes += len;
    ++msgs;
    return len;
}
