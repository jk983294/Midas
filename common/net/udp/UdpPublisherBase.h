#ifndef MIDAS_UDP_PUBLISHER_BASE_H
#define MIDAS_UDP_PUBLISHER_BASE_H

#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/intrusive_ptr.hpp>
#include <string>
#include "net/buffer/ConstBuffer.h"
#include "net/channel/Channel.h"
#include "net/common/NetworkHelper.h"

using namespace std;

namespace midas {

template <typename Derived>
class UdpPublisherBase : public Member {
public:
    static const size_t PublisherReceiveBufferSize = 4096;  // 4K
    typedef boost::intrusive_ptr<Derived> SharedPtr;
    typedef boost::function<size_t(const ConstBuffer&, SharedPtr)> TDataCallback;
    typedef boost::function<size_t(const ConstBuffer&, void*, SharedPtr)> TRequestCallback;

    UdpAddress address;
    CChannel& channel;
    boost::asio::ip::udp::socket skt;
    boost::asio::ip::udp::endpoint senderEndpoint;
    MutableBuffer buffer{PublisherReceiveBufferSize, HeapAllocator()};
    size_t peekBytesSent{0};
    TDataCallback tapInCallback{tap_in_callback()};
    TRequestCallback requestCallback{request_callback()};

public:
    UdpPublisherBase(const string& ip, const string& port, const string& itf, CChannel& output_channel,
                     const string& cfg)
        : channel(output_channel), skt(output_channel.iosvc, boost::asio::ip::udp::v4()) {
        this->configPath = cfg;
        this->netProtocol = NetProtocol::p_udp;
        set_name(ip + ":" + port);
        address = UdpAddress(ip, port, !itf.empty() ? itf : Config::instance().get<string>(cfg + ".bind_address", ""));
        resolve(address);
    }

    ~UdpPublisherBase() {}

    static SharedPtr new_instance(const string& ip, const string& port, const string& interface, CChannel& output,
                                  const string& cfg = Derived::default_config_path(), bool startPublisher = true) {
        SharedPtr publisher(new Derived(ip, port, interface, output, cfg));
        if (startPublisher) publisher->start();
        return publisher;
    }

    static SharedPtr new_instance(const string& ip, const string& port, CChannel& output,
                                  const string& cfg = Derived::default_config_path(), bool startPublisher = true) {
        return new_instance(ip, port, "", output, cfg, startPublisher);
    }

    bool is_multicast() { return address.multicast; }

    void respond(void* requestHandle, const ConstBuffer& msg) {
        if (is_closed()) return;
        skt.send_to(msg, *static_cast<boost::asio::ip::udp::endpoint*>(requestHandle));
        add_sent_msg_2_stat(msg.size());
    }

    void start() {
        try {
            set_socket_options();
            connect();
            set_state(NetState::Connected);
            channel.join<Derived>(SharedPtr((Derived*)this));
            channel.work(1);  // at least one service thread to run
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error init socket" << this->get_name() << " : " << e.what());
            this->set_state(NetState::Closed);
        }
    }

    virtual void stop() {
        set_state(NetState::Closed);
        close();
        channel.leave<Derived>(SharedPtr((Derived*)this));
    }

    void stats(ostream& os) {
        Member::stats(os);
        os << "interface  " << (!address.interface.empty() ? address.interface : string("default")) << '\n'
           << "direction  outbound" << '\n';

        if (skt.is_open()) {
            try {
                os << "local  " << skt.local_endpoint() << '\n';

                {
                    boost::asio::socket_base::receive_buffer_size option;
                    skt.get_option(option);
                    os << "receive_buffer_size " << option.value() << '\n';
                }
                {
                    boost::asio::socket_base::send_buffer_size option;
                    skt.get_option(option);
                    os << "send_buffer_size " << option.value() << '\n';
                }
                {
                    boost::asio::ip::multicast::hops ttl;
                    skt.get_option(ttl);
                    os << "TTL " << ttl.value() << '\n';
                }
            } catch (boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error get socket statistics for " << get_name() << " : " << e.what());
            }
        }
        os << "peek out " << peekBytesSent << '\n';
    }

    static void request_callback(const TRequestCallback& cb) { request_callback() = cb; }

    void request_callback(const TRequestCallback& cb, const CustomCallbackTag&) { requestCallback = cb; }

    static void tap_in_callback(const TDataCallback& cb) { tap_in_callback() = cb; }

    void tap_in_callback(const TDataCallback& cb, const CustomCallbackTag&) { tapInCallback = cb; }

    static void default_config_path(const string& p) { Derived::default_config_path() = p; }

protected:
    void set_socket_options() {
        if (!skt.is_open()) skt.open(boost::asio::ip::udp::v4());

        boost::system::error_code e;

        int recvBufferSize = Config::instance().get<int>(configPath + ".rcvbuf", 0);
        if (recvBufferSize) {
            skt.set_option(boost::asio::socket_base::receive_buffer_size(recvBufferSize), e);
        }

        int sendBufferSize = Config::instance().get<int>(configPath + ".sndbuf", 0);
        if (sendBufferSize) {
            skt.set_option(boost::asio::socket_base::send_buffer_size(sendBufferSize));
        }

        bool enableLoopback = Config::instance().get<bool>(configPath + ".enable_loopback", false);
        if (enableLoopback) {
            skt.set_option(boost::asio::ip::multicast::enable_loopback(enableLoopback));
        }

        if (address.multicast) {
            int ttl = Config::instance().get<int>(configPath + ".ttl", -1);
            if (ttl == 1) {
                ttl = 0;
            }
            if (ttl != -1) {
                skt.set_option(boost::asio::ip::multicast::hops(ttl));
            }
        }
        skt.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    }

    void connect() {
        if (is_multicast()) {
            if (!address.bindIp.empty()) {
                boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address.bindIp),
                                                        address.bindPort);
                skt.bind(endpoint);
                boost::asio::ip::address_v4 outItf = boost::asio::ip::address_v4::from_string(address.bindIp);
                skt.set_option(boost::asio::ip::multicast::outbound_interface(outItf));
            }

            boost::asio::ip::udp::endpoint e1(boost::asio::ip::address_v4::from_string(address.ip), address.port);
            skt.connect(e1);
            MIDAS_LOG_INFO("prepared multicast publisher to "
                           << address.host << ":" << address.service << ". outbound interface: "
                           << (!address.interface.empty() ? address.interface : string("default")));
        } else {
            if (!address.bindIp.empty()) {
                boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address.bindIp),
                                                        address.bindPort);
                skt.bind(endpoint);
            } else {
                boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), address.bindPort);
                skt.bind(endpoint);
            }

            skt.async_receive_from(
                buffer, senderEndpoint,
                boost::bind(&UdpPublisherBase::on_receive_from, SharedPtr((Derived*)this),
                            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            MIDAS_LOG_INFO("prepared unicast publisher on port: "
                           << address.port << ". bind to interface: "
                           << (!address.bindIp.empty() ? address.bindIp : string("default")));
        }
    }

    void close() {
        try {
            if (skt.is_open()) skt.close();
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error close socket: " << get_name() << " : " << e.what());
        }
    }

    void on_receive_from(const boost::system::error_code& ec, size_t byteReceived) {
        if (!ec) {
            buffer.store(byteReceived);
            if (requestCallback) {
                requestCallback(buffer, static_cast<void*>(&senderEndpoint), (Derived*)this);
                if (tapInCallback) {
                    tapInCallback(buffer, (Derived*)this);
                }
            }
            add_recv_msg_2_stat(byteReceived);
            buffer.clear();

            try {
                skt.async_receive_from(
                    buffer, senderEndpoint,
                    boost::bind(&UdpPublisherBase::on_receive_from, SharedPtr((Derived*)this),
                                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error reading socket: " << get_name() << " : " << e.what());
                stop();
            }
        } else {
            MIDAS_LOG_ERROR("error reading socket: " << get_name());
        }
    }

    static TRequestCallback& request_callback() {
        static TRequestCallback _callback;
        return _callback;
    }

    static TDataCallback& tap_in_callback() {
        static TDataCallback _callback;
        return _callback;
    }

    void resolve(UdpAddress& addr) {
        try {
            boost::asio::ip::udp::resolver resolver(channel.iosvc);
            {
                boost::asio::ip::udp::resolver::query q(addr.host, addr.service);
                boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(q);

                addr.ip = endpoint.address().to_string();
                addr.port = endpoint.port();
                addr.multicast = endpoint.address().is_multicast();
            }

            string bindIp, bindPort = "0", itf;
            if (!addr.interface.empty()) {
                split_host_port_interface(addr.interface, bindIp, bindPort, itf);

                if (!bindIp.empty() && ::isalpha(bindIp[0])) {
                    try {
                        string ifAddress;
                        NetworkHelper helper;
                        helper.lookup_interface(bindIp, ifAddress);
                        bindIp = ifAddress;
                    } catch (const MidasException& e) {
                        MIDAS_LOG_ERROR("error for interface lookup: " << bindIp << " : " << e.what());
                    }
                }
            }

            addr.bindIp = bindIp;
            if (bindPort != "0") {
                boost::asio::ip::udp::resolver::query q(bindPort);
                boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(q);
                addr.bindPort = endpoint.port();
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error resolve: " << addr.host << ":" << addr.service << " : " << e.what());
        }
    }
};
}

#endif
