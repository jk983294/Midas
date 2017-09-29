#ifndef MIDAS_STREAM_PUBLISHER_BASE_H
#define MIDAS_STREAM_PUBLISHER_BASE_H

#include <tbb/atomic.h>
#include <boost/algorithm/string.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/write.hpp>
#include "net/buffer/ConstBuffer.h"
#include "net/buffer/ConstBufferSequence.h"
#include "net/buffer/MutableBuffer.h"
#include "net/channel/Channel.h"
#include "process/admin/AdminHandler.h"

namespace midas {

// base for all stream based publisher
template <typename ProtocolType, typename Derived>
class StreamPublisherBase : public Member {
public:
    static const size_t PublisherRecvBufferSize = 10240;
    typedef boost::intrusive_ptr<Derived> SharedPtr;
    // if return false, connection will close and leave channel
    typedef boost::function<bool(SharedPtr)> TConnectCallback;
    typedef boost::function<bool(SharedPtr)> TDisconnectCallback;
    typedef boost::function<size_t(const ConstBuffer&, SharedPtr)> TDataCallback;

    CChannel& channel;
    TConnectCallback connectCallback;
    TDisconnectCallback disconnectCallback;
    TDataCallback dataCallback;
    TDataCallback tapInCallback;

    typename ProtocolType::socket skt;
    boost::asio::strand _strand;  // associated to this publisher to ensure buffers are read/write in order
    size_t peekBytesRecv{0}, peekBytesSent{0};  // record peak volume
    MutableBuffer readBuffer;
    tbb::atomic<bool> stopFlag;

public:
    static SharedPtr new_instance(CChannel& c, const string& cfg = Derived::default_config_path()) {
        return SharedPtr(new Derived(c, cfg));
    }

    // called by acceptor to init actor
    virtual void start() {
        try {
            if (get_name().empty()) {
                // build publisher id
                static size_t publisherIdIndex = 0;
                ostringstream os;
                os << skt.local_endpoint() << "#" << publisherIdIndex++;
                set_name(boost::trim_copy(os.str()));
            }

            int recvBufferSize = Config::instance().get<int>(configPath + ".rcvbuf", 0);
            if (recvBufferSize) {
                boost::system::error_code e;
                skt.set_option(boost::asio::socket_base::receive_buffer_size(recvBufferSize), e);
                if (e) {
                    MIDAS_LOG_ERROR("error set socket recv buffer size " << get_name() << " to " << recvBufferSize);
                }
            }

            int sendBufferSize = Config::instance().get<int>(configPath + ".sndbuf", 0);
            if (sendBufferSize) {
                boost::system::error_code e;
                skt.set_option(boost::asio::socket_base::send_buffer_size(sendBufferSize), e);
                if (e) {
                    MIDAS_LOG_ERROR("error set socket send buffer size " << get_name() << " to " << sendBufferSize);
                }
            }

            if (!connectCallback((Derived*)this)) {
                shutdown();
                return;
            }

            skt.async_read_some(readBuffer,
                                _strand.wrap(boost::bind(&StreamPublisherBase::on_read, SharedPtr((Derived*)this),
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred)));
            _strand.post(boost::bind(&StreamPublisherBase::init, SharedPtr((Derived*)this)));
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error init socket" << get_name() << " : " << e.what());
            state = Closed;
        }
    }
    typename ProtocolType::socket& socket() { return skt; }

    void stats(ostream& os) {
        Member::stats(os);
        if (skt.is_open()) {
            try {
                os << "address:      " << skt.remote_endpoint() << '\n'
                   << "local:        " << skt.local_endpoint() << '\n';
                {
                    boost::asio::socket_base::receive_buffer_size option;
                    skt.get_option(option);
                    os << "recv buffer size:    " << option.value() << '\n';
                }
                {
                    boost::asio::socket_base::send_buffer_size option;
                    skt.get_option(option);
                    os << "send buffer size:    " << option.value() << '\n';
                }
                try {
                    boost::asio::socket_base::keep_alive option;
                    skt.get_option(option);
                    os << "keep alive:    " << (option.value() ? "true" : "false") << '\n';
                } catch (...) {
                }
            } catch (boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error getting socket statistics for " << get_name() << " : " << e.what());
            }
            os << "peek recv:      " << peekBytesRecv << '\n' << "peek send:        " << peekBytesSent << '\n';
        }
    }
    void shutdown() {
        MIDAS_LOG_INFO("shutdown connection: " << get_name());
        try {
            skt.close();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error close socket" << get_name() << " : " << e.what());
        }
        stop();
    }

    static void connect_callback(const TConnectCallback& callback) { connect_callback() = callback; }
    void connect_callback(const TConnectCallback& callback, const CustomCallbackTag&) { connectCallback = callback; }
    static void disconnect_callback(const TDisconnectCallback& callback) { disconnect_callback() = callback; }
    void disconnect_callback(const TDisconnectCallback& callback, const CustomCallbackTag&) {
        disconnectCallback = callback;
    }
    static void data_callback(const TDataCallback& callback) { data_callback() = callback; }
    void data_callback(const TDataCallback& callback, const CustomCallbackTag&) { dataCallback = callback; }
    static void tap_in_callback(const TDataCallback& callback) { tap_in_callback() = callback; }
    void tap_in_callback(const TDataCallback& callback, const CustomCallbackTag&) { tapInCallback = callback; }
    static void default_config_path(const string& p) { Derived::default_config_path() = p; }

protected:
    StreamPublisherBase(CChannel& c, const string& cfg)
        : channel(c),
          connectCallback(connect_callback()),
          disconnectCallback(disconnect_callback()),
          dataCallback(data_callback()),
          tapInCallback(tap_in_callback()),
          skt(c.iosvc),
          _strand(c.iosvc),
          readBuffer(PublisherRecvBufferSize, HeapAllocator()) {
        configPath = cfg;
        stopFlag = false;
    }
    ~StreamPublisherBase() {}
    virtual void init() {
        if (state == Created) {
            state = Connected;
            channel.join<Derived>(SharedPtr((Derived*)this));
            MIDAS_LOG_INFO("new client connection " << get_name() << " created (" << skt.local_endpoint() << " #" << id
                                                    << " <-- " << skt.remote_endpoint() << ")");
        }
    }
    virtual void stop() {
        MIDAS_LOG_INFO("close connection " << get_name());
        disconnectCallback((Derived*)this);
        channel.leave<Derived>(SharedPtr((Derived*)this));
        state = Closed;
    }

    // callback when some data recv from downstream connection
    void on_read(const boost::system::error_code& e, size_t transferred) {
        if (!e) {
            readBuffer.store(transferred);
            size_t processed = dataCallback(readBuffer, (Derived*)this);
            if (tapInCallback) {
                tapInCallback(ConstBuffer(readBuffer, processed), (Derived*)this);
            }

            // allocate new buffer and leave old buffer for client
            readBuffer = MutableBuffer(readBuffer, processed);
            add_recv_msg_2_stat(processed);
            if (transferred > peekBytesRecv) peekBytesRecv = transferred;

            try {
                skt.async_read_some(readBuffer,
                                    _strand.wrap(boost::bind(&StreamPublisherBase::on_read, SharedPtr((Derived*)this),
                                                             boost::asio::placeholders::error,
                                                             boost::asio::placeholders::bytes_transferred)));
            } catch (const boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error read socket" << get_name() << " : " << e.what());
                close();
            }
        } else {
            close();
        }
    }

    bool closed() const { return stopFlag; }

    void close() {
        if (!stopFlag.compare_and_swap(true, false)) {
            _strand.post(boost::bind(&StreamPublisherBase::shutdown, SharedPtr((Derived*)this)));
        }
    }

private:
    static bool default_connect_callback() { return true; }
    static TConnectCallback& connect_callback() {
        static TConnectCallback _callback = MIDAS_STATIC_BIND0(&StreamPublisherBase::default_connect_callback);
        return _callback;
    }
    static bool default_disconnect_callback() { return true; }
    static TDisconnectCallback& disconnect_callback() {
        static TDisconnectCallback _callback = MIDAS_STATIC_BIND0(&StreamPublisherBase::default_disconnect_callback);
        return _callback;
    }
    static bool default_data_callback(const ConstBuffer& msg) { return msg.size(); }
    static TDataCallback& data_callback() {
        static TDataCallback _callback = MIDAS_STATIC_BIND1(&StreamPublisherBase::default_data_callback);
        return _callback;
    }
    static TDataCallback& tap_in_callback() {
        static TDataCallback _callback;
        return _callback;
    }
};
}

#endif
