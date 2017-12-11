#ifndef MIDAS_TCP_PUBLISHER_BASE_H
#define MIDAS_TCP_PUBLISHER_BASE_H

#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <sstream>
#include "midas/MidasConfig.h"
#include "net/channel/Channel.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

template <typename StreamBase>
class TcpPublisherBase : public StreamBase {
public:
    TcpPublisherBase(CChannel& output_channel, const string& cfg) : StreamBase(output_channel, cfg) {
        this->netProtocol = NetProtocol::p_tcp_primary;
    }

    virtual void start() {
        try {
            {
                boost::asio::ip::tcp::resolver resolver(this->channel.iosvc);
                boost::asio::ip::tcp::resolver::iterator endpoint_iterator =
                    resolver.resolve(this->skt.remote_endpoint());

                // publisher id
                ostringstream oss;
                if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
                    string hostname = endpoint_iterator->host_name();
                    oss << hostname.substr(0, hostname.find('.')) << ":" << endpoint_iterator->endpoint().port();
                } else {
                    oss << this->skt.remote_endpoint();
                }
                this->set_name(oss.str());
            }

            {
                // enable keep alive
                bool isKeepAlive = Config::instance().get<bool>(this->configPath + ".keepalive", true);
                boost::system::error_code ec;
                this->skt.set_option(boost::asio::socket_base::keep_alive(isKeepAlive), ec);
                if (ec) MIDAS_LOG_ERROR("error turn on keep alive option for " << this->get_name());
            }

            {
                // enable tcp no delay
                bool isNoDelay = Config::instance().get<bool>(this->configPath + ".nodelay", true);
                boost::system::error_code ec;
                this->skt.set_option(boost::asio::ip::tcp::no_delay(isNoDelay), ec);
                if (ec) MIDAS_LOG_ERROR("error turn on no delay option for " << this->get_name());
            }
            StreamBase::start();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error init socket" << this->get_name() << " : " << e.what());
            this->set_state(NetState::Closed);
        }
    }
};
}

#endif
