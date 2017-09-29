#ifndef MIDAS_TCP_PUBLISHER_ASYNC_H
#define MIDAS_TCP_PUBLISHER_ASYNC_H

#include "net/common/PublisherAsync.h"
#include "net/common/StreamPublisherBase.h"
#include "net/tcp/TcpPublisherBase.h"

namespace midas {

static const size_t DefaultTcpWriteThreshold = 100 * 1024 * 1024;

template <typename PublisherTag = DefaultTcpPublisherTag, bool ThreadPerConn = true,
          size_t WriteThreshold = DefaultTcpWriteThreshold>
class TcpPublisherAsync
    : public TcpPublisherBase<
          PublisherAsync<boost::asio::ip::tcp, TcpPublisherAsync<PublisherTag, ThreadPerConn, WriteThreshold>,
                         StreamPublisherBase, ThreadPerConn, WriteThreshold>> {
public:
    typedef TcpPublisherBase<
        PublisherAsync<boost::asio::ip::tcp, TcpPublisherAsync<PublisherTag, ThreadPerConn, WriteThreshold>,
                       StreamPublisherBase, ThreadPerConn, WriteThreshold>>
        PublisherBase;

public:
    void deliver(const ConstBuffer& msg) { PublisherBase::deliver(msg); }

    static string& default_config_path() {
        static string _path{"midas.io.tcp_out"};
        return _path;
    }

public:
    TcpPublisherAsync(CChannel& c, const string& cfg) : PublisherBase(c, cfg) {}
};
}

#endif
