#ifndef MIDAS_TCP_ADMIN_H
#define MIDAS_TCP_ADMIN_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include "midas/MidasConstants.h"
#include "net/channel/Channel.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

struct DefaultAdminConnectionTag {};

class TcpAdmin : public Member {
public:
    typedef boost::intrusive_ptr<TcpAdmin> SharedPtr;
    typedef boost::function<size_t(const ConstBuffer&, SharedPtr)> TDataCallback;

    CChannel& channel;
    boost::asio::ip::tcp::socket skt;
    boost::asio::streambuf request;  // used to read admin command

public:
    ~TcpAdmin() {}

    void start() {
        boost::asio::ip::tcp::resolver resolver(channel.iosvc);
        boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(skt.remote_endpoint());

        ostringstream os;
        if (endpoint != boost::asio::ip::tcp::resolver::iterator()) {
            os << endpoint->host_name() << ":" << endpoint->endpoint().port();
        } else {
            os << skt.remote_endpoint();
        }

        set_name(os.str());
        channel.join<TcpAdmin>(this);
        MIDAS_LOG_INFO("new admin connection (" << skt.local_endpoint().address().to_string() << ":"
                                                << skt.local_endpoint().port() << " # " << id << " <-- "
                                                << skt.remote_endpoint().address().to_string() << ":"
                                                << skt.remote_endpoint().port() << ")");

        boost::asio::async_read_until(
            skt, request, MidasConstants::instance().AdminDelimiter,
            boost::bind(&TcpAdmin::on_admin_read, SharedPtr(this), boost::asio::placeholders::error));
    }

    boost::asio::ip::tcp::socket& socket() { return skt; }

    static SharedPtr new_instance(CChannel& adminChannel, const string& cfg = default_config_path()) {
        return SharedPtr(new TcpAdmin(adminChannel, cfg));
    }

    void deliver(const ConstBuffer& msg) {
        try {
            if (boost::asio::write(skt, msg) != msg.size()) {
                MIDAS_LOG_ERROR("error write to admin call " << get_name());
                close();
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error write admin call " << get_name() << " : " << e.what());
            close();
        }
    }

    void close() {
        if (skt.is_open()) {
            try {
                skt.close();
            } catch (boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error close socket " << get_name() << " : " << e.what());
            }
        }
        channel.leave<TcpAdmin>(this);
    }

    void on_admin_read(const boost::system::error_code& e) {
        if (!e) {
            data_callback()(request.data(), SharedPtr(this));
            request.consume(request.size());
            boost::asio::async_read_until(
                skt, request, MidasConstants::instance().AdminDelimiter,
                boost::bind(&TcpAdmin::on_admin_read, SharedPtr(this), boost::asio::placeholders::error));
        } else {
            close();
        }
    }

    static void data_callback(const TDataCallback& callback) { data_callback() = callback; }

    static string& default_config_path() {
        static string _path("midas.net.admin");
        return _path;
    }

    static void default_config_path(const string& path) { default_config_path() = path; }

private:
    TcpAdmin(CChannel& adminChannel, const string& cfg) : channel(adminChannel), skt(adminChannel.iosvc) {
        configPath = cfg;
        netProtocol = p_tcp_primary;
    }

    static size_t default_data_callback(const ConstBuffer& msg) { return msg.size(); }

    static TDataCallback& data_callback() {
        static TDataCallback _callback = boost::bind(&TcpAdmin::default_data_callback, _1);
        return _callback;
    }
};
}

#endif
