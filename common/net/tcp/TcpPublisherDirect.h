#ifndef MIDAS_TCP_PUBLISHER_DIRECT_H
#define MIDAS_TCP_PUBLISHER_DIRECT_H

#include "net/common/PublisherDirect.h"
#include "net/common/StreamPublisherBase.h"
#include "net/tcp/TcpPublisherBase.h"

namespace midas {

/**
 * this will send msg directly. the deliver call will call a locking write to deliver data
 * no msg caching and will block if client cannot handle the amount
 * this can be used with small numbers of client without data loss
 */
template <typename PublisherTag = DefaultTcpPublisherTag>
class TcpPublisherDirect
    : public TcpPublisherBase<
          PublisherDirect<boost::asio::ip::tcp, TcpPublisherDirect<PublisherTag>, StreamPublisherBase>> {
public:
    typedef TcpPublisherBase<
        PublisherDirect<boost::asio::ip::tcp, TcpPublisherDirect<PublisherTag>, StreamPublisherBase>>
        PublisherBase;
    template <typename P, typename D>
    friend class StreamPublisherBase;

public:
    void deliver(const ConstBuffer& msg) { PublisherBase::deliver(msg); }

    static string& default_config_path() {
        static string _path{"midas.io.tcp_out"};
        return _path;
    }

private:
    TcpPublisherDirect(CChannel& c, const string& cfg) : PublisherBase(c, cfg) {}
};
}

#endif
