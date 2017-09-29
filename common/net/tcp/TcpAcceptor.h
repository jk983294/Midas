#ifndef MIDAS_TCP_ACCEPTOR_H
#define MIDAS_TCP_ACCEPTOR_H

#include <tbb/concurrent_vector.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind/placeholders.hpp>
#include "net/channel/Channel.h"
#include "net/channel/ChannelPublisherBase.h"
#include "utils/ConvertHelper.h"
#include "utils/log/Log.h"

namespace midas {

/**
 * it will listen on the port and init publisher objects for each newly established connection
 */
template <typename PublisherType>
class TcpAcceptor : public ChannelPublisherBase {
public:
    typedef typename PublisherType::SharedPtr PublisherPtr;
    typedef typename boost::intrusive_ptr<TcpAcceptor> SharedPtr;
    typedef boost::function<bool(PublisherPtr)> TAcceptCallback;

    CChannel& channel;
    boost::asio::ip::tcp::acceptor acceptor;
    ssize_t acceptorSlot;
    TAcceptCallback acceptCallback;

public:
    TcpAcceptor(int port, CChannel& output, const string& cfg = string(), const string& name = string())
        : channel(output),
          acceptor(output.iosvc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          acceptorSlot(output.get_slot()),
          acceptCallback(get_accept_callback()) {
        configPath = (!cfg.empty() ? cfg : PublisherType::default_config_path());

        try {
            set_name(name + string(":") + int2str(acceptor.local_endpoint().port()));
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error get port from TCP acceptor on " << port << " - " << e.what());
            set_name(name + string(":") + int2str(port));
        }

        netProtocol = p_tcp_primary;
        displayStat = false;
        MIDAS_LOG_INFO("a tcp acceptor created: " << this->get_name());
    }

    TcpAcceptor(const string& svc, CChannel& output, const string& cfg = string(), const string& name = string())
        : channel(output),
          acceptor(output.iosvc),
          acceptorSlot(output.get_slot()),
          acceptCallback(get_accept_callback()) {
        if (svc.empty() || svc == "0") {
            acceptor.open(boost::asio::ip::tcp::v4());
        } else {
            boost::asio::ip::tcp::resolver resolver(output.iosvc);
            boost::asio::ip::tcp::resolver::query query(svc);
            boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

            acceptor.open(endpoint.protocol());
            acceptor.set_option(boost::asio::socket_base::reuse_address(true));
            acceptor.bind(endpoint);
        }

        acceptor.listen();

        configPath = (!cfg.empty() ? cfg : PublisherType::default_config_path());

        if (svc.empty() || svc == "0") {
            try {
                set_name(name + string(":") + int2str(acceptor.local_endpoint().port()));
            } catch (const boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error get port from TCP acceptor on " << svc << " - " << e.what());
                set_name(name + string(":") + svc);
            }
        } else {
            set_name(name + string(":") + svc);
        }

        netProtocol = p_tcp_primary;
        displayStat = false;
        MIDAS_LOG_INFO("a tcp acceptor created: " << this->get_name());
    }

    ~TcpAcceptor() { channel.clear_slot(acceptorSlot); }

    static SharedPtr new_instance(int port, CChannel& output, bool startAcceptor = true, const string& cfg = string(),
                                  const string& name = string()) {
        SharedPtr acc;
        try {
            acc.reset(new TcpAcceptor(port, output, cfg, name));
            if (acc && startAcceptor) acc->start();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error create TCP acceptor on " << port << " - " << e.what());
            acc.reset();
        }
        return acc;
    }

    static SharedPtr new_instance(const string& svc, CChannel& output, bool startAcceptor = true,
                                  const string& cfg = string(), const string& name = string("localhost")) {
        SharedPtr acc;
        try {
            acc.reset(new TcpAcceptor(svc, output, cfg, name));
            if (acc && startAcceptor) acc->start();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error create TCP acceptor on " << svc << " - " << e.what());
            acc.reset();
        }
        return acc;
    }

    void start() override {
        // create new publisher for new connection request
        PublisherPtr newConn = PublisherType::new_instance(channel, configPath);
        newConn->userData = userData;

        // async accept new connections
        acceptor.async_accept(newConn->socket(), boost::bind(&TcpAcceptor::on_accept, this, newConn, _1));
        channel.join<TcpAcceptor>(this);
    }

    void stop() override { channel.leave<TcpAcceptor>(this); }

    void on_accept(PublisherPtr connection, const boost::system::error_code& e) {
        if (!e) {
            if (acceptCallback(connection)) {
                connection->start();
                channel.template join_slot<PublisherType>(connection, acceptorSlot);
            }

            // create new publisher for new connection request
            PublisherPtr newConn = PublisherType::new_instance(channel, configPath);
            newConn->userData = userData;

            // async accept new connections
            acceptor.async_accept(newConn->socket(), boost::bind(&TcpAcceptor::on_accept, this, newConn, _1));
        }
    }

    void deliver(const ConstBuffer& msg, ssize_t slot) {
        if (slot != -1) {
            channel.deliver_slot(msg, acceptorSlot);
        }
    }

    void set_callback_func(boost::function<void(ConstBuffer, bool)>& f) override {
        f = [&, this](ConstBuffer buffer, bool) { this->deliver(buffer); };
    }

    // not thread safe, must called before channel start
    static void accept_callback(const TAcceptCallback& callback) { get_accept_callback() = callback; }

    void accept_callback(const TAcceptCallback& callback, const CustomCallbackTag&) { acceptCallback = callback; }

    template <typename Payload>
    void on_data_call_back(const Payload& payload) {
        channel.deliver(payload.const_buffer());
    }

    void deliver(const ConstBuffer msg) { channel.deliver(msg); }

private:
    static bool default_accept_callback() { return true; }
    static TAcceptCallback& get_accept_callback() {
        static TAcceptCallback cb = boost::bind(&TcpAcceptor::default_accept_callback);
        return cb;
    }
};
}

#endif
