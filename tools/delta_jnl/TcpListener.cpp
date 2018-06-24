#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "TcpListener.h"

using namespace std;

TcpListener::TcpListener(const string &service_, bool noDelay_)
    : service(service_), fd(socket(AF_INET, SOCK_STREAM, 0)), noDelay(noDelay_) {
    if (fd < 0) {
        cerr << "socket() failed: " << strerror(errno) << "\n";
        return;
    }

    int n = noDelay ? 1 : 0;
    int rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof(n));
    if (rc < 0) {
        cerr << "setsockopt(TCP_NODELAY) failed: " << strerror(errno) << "\n";
        return;
    }

    int long mask(fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    rc = fcntl(fd, F_SETFL, mask);
    if (rc < 0) {
        cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << "\n";
        return;
    }

    int val = 1;
    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (rc < 0) {
        cerr << "setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << "\n";
        return;
    }

    unsigned short portNum = port();
    if (!portNum) return;

    struct sockaddr_in inAddr;
    memset(&inAddr, 0, sizeof(inAddr));
    inAddr.sin_family = AF_INET;
    inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inAddr.sin_port = htons(portNum);

    rc = bind(fd, (struct sockaddr *)&inAddr, sizeof(inAddr));
    if (rc < 0) {
        cerr << "bind() failed: " << strerror(errno) << "\n";
        return;
    }

    rc = listen(fd, 5);
    if (rc < 0) {
        cerr << "listen() failed: " << strerror(errno) << "\n";
        return;
    }

    isDeaf = false;
}
TcpListener::~TcpListener() {
    if (fd < 0) return;
    close(fd);
}

unsigned short TcpListener::port() {
    unsigned short portNum = atoi(service.c_str());
    if (portNum) return portNum;

    servent &s(*getservbyname(service.c_str(), "tcp"));
    if (&s) return ntohs(s.s_port);
    return 0;
}

bool TcpListener::admit(int &fd_, string &address) const {
    struct sockaddr_in inAddr;
    memset(&inAddr, 0, sizeof(inAddr));
    socklen_t len = sizeof(inAddr);
    fd_ = accept(fd, (struct sockaddr *)&inAddr, &len);
    if (fd_ < 0) {
        if (errno != EAGAIN) cerr << "accept() failed: " << strerror(errno) << "\n";
        return false;
    }

    int n = noDelay ? 0 : 1;
    socklen_t sL = sizeof(n);
    int rc = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sL);
    if (rc < 0) {
        cerr << "setsockopt(TCP_NODELAY) failed: " << strerror(errno) << "\n";
    }

    if ((n != 0) != noDelay) {
        cerr << "unexpected TCP_NODELAY " << n << "\n";
    }

    unsigned short port = ntohs(inAddr.sin_port);
    struct hostent *h;
    h = gethostbyaddr((char *)&inAddr.sin_addr, sizeof(inAddr.sin_addr), AF_INET);
    if (!h) {
        cerr << "gethostbyaddr() failed: " << strerror(errno) << "\n";
        return false;
    }

    char addrStr[80];
    sprintf(addrStr, "%s.%d", h->h_name, port);
    address = string(addrStr);
    return true;
}
