#ifndef MIDAS_TCP_INPUT_H
#define MIDAS_TCP_INPUT_H

#include <iostream>
#include <string>

using namespace std;

class TcpInput {
public:
    enum Connection { Trying, Active, Broken };
    Connection state{Broken};
    const string host;
    const string port;
    char* const beginBuffer;
    const char* const endBuffer;
    const char* const warningTrack;
    const char* beginData;
    char* endData;
    const timeval birth;
    long long useconds{0};
    long long msgs{0};
    timeval then[2];
    long long usecondsThen[2];
    long long msgsThen[2];
    long long bytes{0};

public:
    TcpInput(int fd, const string& host_, const string& service_, const string& iface, bool noDelay_,
             const timeval& now);
    virtual ~TcpInput();

    bool isTrying() const { return state == Connection::Trying; }
    bool isActive() const { return state == Connection::Active; }
    bool isBroken() const { return state == Connection::Broken; }

    void occasional(const timeval& now);

    void serviceRead(int fd);
    void connectDone(int fd);

    static int socket_fd();

    /**
     * move data beginning at a block boundary to the buffer
     * a block boundary is a point in the data from which the data can be deblocked
     * @return > 0 if a block was moved
     *         = 0 if no block moved
     *         = -1 for ghastly error
     */
    virtual int getBlock(char*, const timeval&) = 0;

    static int bufferLen() { return 3000000; }
    static int maxBlockLen() { return 80000; }

private:
    void becomeClient(int fd);

    static bool setBlockingIo(int fd, bool yes);
    static bool setKeepAlive(int fd);
    static bool setInterface(int fd, const string& iface);
    static bool setNoDelay(int fd, bool yes);
};

class DeltaPass : public TcpInput {
public:
    DeltaPass(int fd, const string& host_, const string& service_, const string& iface, bool noDelay_,
              const timeval& now);

    int getBlock(char*, const timeval&) override;
};

#endif
