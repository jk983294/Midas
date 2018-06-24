#ifndef MIDAS_TCP_LISTENER_H
#define MIDAS_TCP_LISTENER_H

#include <iostream>
#include <string>

using namespace std;

class TcpListener {
public:
    TcpListener(const string& service_, bool noDelay_);
    ~TcpListener();

    bool admit(int& fd_, string& address) const;

    unsigned short port();

public:
    const string service;
    const int fd;
    bool isDeaf{true};
    bool noDelay;
};

#endif
