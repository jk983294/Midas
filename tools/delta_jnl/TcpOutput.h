#ifndef MIDAS_TCP_OUTPUT_H
#define MIDAS_TCP_OUTPUT_H

#include <netinet/tcp.h>
#include <ctime>
#include <iostream>
#include <string>

using namespace std;

class TcpOutput {
public:
    const string address;
    const timeval birth;
    const long long totalBytesAtBirth;
    long long totalBytes;

public:
    TcpOutput(int fd, const string& addr_, long long bytes, const timeval& now);

    long pendingBytes(long long totalBytes_) const { return totalBytes_ - totalBytes; }
    long long bytes() const { return totalBytes - totalBytesAtBirth; }
    long freeBytes(int bufLen, long long totalBytes_) const {
        return bufLen - static_cast<int>(totalBytes_ - totalBytes);
    }

    static void gentleClose(int fd);
    bool output(int fd, char* bufPtr, int bufLen, long long totalBytes_);
    bool input(int fd) const;
};

#endif
