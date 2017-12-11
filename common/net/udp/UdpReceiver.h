#ifndef MIDAS_UDP_RECEIVER_H
#define MIDAS_UDP_RECEIVER_H

#include <boost/asio/ip/udp.hpp>
#include <string>
#include "net/buffer/ConstBuffer.h"
#include "net/buffer/ConstBufferSequence.h"
#include "net/channel/Channel.h"
#include "net/common/NetworkHelper.h"
#include "net/common/TimerMember.h"
#include "process/admin/AdminHandler.h"

using namespace std;

#ifdef MIDAS_ENABLE_RECVMMSG
#include <syscall.h>
static inline int recvmmsg(int fd, struct mmsghdr* mmsg, unsigned vlen, unsigned flags) {
    return syscall(__NR_recvmmsg, fd, mmsg, vlen, flags);
}
#endif

namespace midas {

static const size_t DefaultUdpRecvBufferSize = 8 * 1024;

// object to listen and recv incoming msg
template <typename Tag = DefaultUdpReceiverTag, size_t BufferSize = DefaultUdpRecvBufferSize,
          typename Allocator = PoolAllocator<BufferSize, Tag>, bool UseStrand = false>
class UdpReceiver : public Member {
public:
    static const int DefaultReceiveBufferSize = 8388608;  // 8M
    template <bool T>
    struct StrandType {
        static const bool value = T;
    };
    typedef typename boost::intrusive_ptr<UdpReceiver> SharedPtr;
    typedef boost::function<size_t(const ConstBuffer&, SharedPtr)> TDataCallback;
    typedef boost::function<size_t(const ConstBufferSequence&, SharedPtr)> TDataSequenceCallback;

    UdpAddress address;
    CChannel& channel;
    TDataCallback dataCallback{data_callback()};
    TDataCallback tapInCallback{tap_in_callback()};
    TDataCallback tapOutCallback{tap_out_callback()};
    TDataSequenceCallback dataSequenceCallback;
    boost::asio::ip::udp::socket skt;
    boost::asio::ip::udp::endpoint senderEndpoint;
    MutableBuffer buffer{BufferSize, Allocator()};
    ConstBufferSequence bufferSequence;
    uint64_t receiveTimeout;
    uint64_t pollingTime;

public:
    UdpReceiver(const string& ip, const string& port, const string& interface, const string& cfg, CChannel& input)
        : channel(input),
          skt(input.iosvc, boost::asio::ip::udp::v4()),
          receiveTimeout(
              Config::instance()
                  .get<boost::posix_time::time_duration>(cfg + ".receive_timeout", boost::posix_time::microseconds(100))
                  .total_nanoseconds()),
          pollingTime(
              Config::instance()
                  .get<boost::posix_time::time_duration>(cfg + ".polling_time", boost::posix_time::microseconds(100))
                  .total_nanoseconds()) {
        configPath = cfg;
        set_name(member_name(ip, port));
        netProtocol = NetProtocol::p_udp;
        address = UdpAddress(
            ip, port, !interface.empty() ? interface : Config::instance().get<string>(cfg + ".bind_address", ""));
        resolve(address);
        buffer.store(reserve_header_size());
    }

    ~UdpReceiver() {}

    // caller can ignore return value since receiver will join and be part of given channel
    static SharedPtr new_instance(const string& ip, const string& port, const string& interface, const string& cfg,
                                  CChannel& input, bool startReceiver = true) {
        SharedPtr receiver(new UdpReceiver(ip, port, interface, cfg, input));
        if (startReceiver) receiver->start();
        return receiver;
    }

    // caller can ignore return value since receiver will join and be part of given channel
    static SharedPtr new_instance(const string& ip, const string& port, const string& interface, CChannel& input,
                                  bool startReceiver = true) {
        return new_instance(ip, port, interface, default_config_path(), input, startReceiver);
    }

    // caller can ignore return value since receiver will join and be part of given channel
    static SharedPtr new_instance(const string& ip, const string& port, CChannel& input, bool startReceiver = true) {
        return new_instance(ip, port, "", default_config_path(), input, startReceiver);
    }

    static string member_name(const string& h, const string& s) { return h + ":" + s; }

    // create new receiver as handler of admin command
    static string admin_new_receiver(const string& cmd, const TAdminCallbackArgs& args, CChannel& input,
                                     const string& group, const string& cfg) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string ipService = args[0];
        string ip, service, interface;

        split_host_port_interface(ipService, ip, service, interface);

        if (ip.empty() || service.empty()) return string("Usage: ") + cmd + " <ip:port[:interface]> [desc]";
        if (input.get_member(member_name(ip, service)))
            return string("connection already exists for: ") + ip + ":" + service;

        // desc and config path
        string s1 = (args.size() > 1 ? args[1] : "");
        string s2 = (args.size() > 2 ? args[2] : "");
        string description(group);
        string adminCfgPath = (cfg.empty() ? default_config_path() : cfg);

        MIDAS_LOG_INFO("add new udp connection: " << ip << ":" << service
                                                  << " interface: " << (!interface.empty() ? interface : "default"));
        SharedPtr receiver = new_instance(ip, service, interface, adminCfgPath, input, false);
        if (receiver) {
            receiver->set_description(description);
            receiver->start();
            if (receiver->is_multicast())
                return string("new connection successfully joined: ") + ip + ":" + service + " (" + description +
                       " config: " + adminCfgPath + ")";
            else
                return string("new connection successfully opened: ") + ip + ":" + service + " (" + description +
                       " config: " + adminCfgPath + ")";
        }
        return string("failed to open connection: ") + ip + ":" + service;
    }

    static string admin_stop_receiver(const string& cmd, const TAdminCallbackArgs& args, CChannel& input) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string name = args[0];
        MemberPtr connection = input.get_member(name);
        if (!connection) return "connection not found.";
        SharedPtr receiver = dynamic_cast<UdpReceiver*>(connection.get());
        receiver->stop();
        if (receiver->is_multicast()) return string("successfully left ") + name;
        return string("successfully closed ") + name;
    }

    static void register_admin(CChannel& input, AdminHandler& handler, const string& group = string(),
                               const string& cfg = string()) {
        string open_cmd{"open_server"};
        string close_cmd{"close_server"};
        if (!group.empty()) {
            open_cmd += "_" + group;
            close_cmd += "_" + group;
        }

        handler.register_callback(
            open_cmd, boost::bind(&UdpReceiver::admin_new_receiver, _1, _2, boost::ref(input), group, cfg),
            "open connection", string("usage: ") + open_cmd + " ip:port[:interface] [description] [cfg]");
        handler.register_callback(close_cmd, boost::bind(&UdpReceiver::admin_stop_receiver, _1, _2, boost::ref(input)),
                                  input, "close connection", "usage: ip:port");
    }

    void start() {
        channel.work(1);
        if (UseStrand)
            channel.mainStrand.post(boost::bind(&UdpReceiver::async_start, SharedPtr(this)));
        else
            async_start();
    }

    void start_non_block() {
        skt.non_blocking(true);
        start();
    }

    // stop receiver and remove it from channel
    void stop() {
        if (UseStrand) {
            channel.mainStrand.post(boost::bind(&UdpReceiver::async_stop, SharedPtr(this)));
        } else
            async_stop();
    }

    bool is_multicast() { return address.multicast; }

    // deliver msg to upstream, only make sense when unicast
    void deliver(const ConstBuffer& msg) {
        if (!skt.is_open()) return;

        try {
            if (skt.send(msg) != msg.size()) {
                MIDAS_LOG_ERROR("error writing to socket " << get_name());
            } else {
                if (tapOutCallback) {
                    tapOutCallback(msg, SharedPtr(this));
                }
                add_sent_msg_2_stat(msg.size());
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error writing to socket " << get_name() << " : " << e.what());
        }
    }

    void stats(ostream& os) {
        Member::stats(os);
        os << "interface  " << (!address.interface.empty() ? address.interface : string("default")) << '\n'
           << "direction  outbound" << '\n'
           << "buffer size  " << BufferSize << '\n';

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
                os << "non-blocking socket " << get_bool_string(skt.non_blocking()) << '\n';
                if (skt.non_blocking()) {
                    os << "polling time " << pollingTime / 1000 << " usec" << '\n';
                    os << "receive timeout " << receiveTimeout / 1000 << " usec" << '\n';
                }
            } catch (boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error get socket statistics for " << get_name() << " : " << e.what());
            }
        }
    }

private:
    void async_start() {
        if (state == NetState::Created || state == NetState::Closed) {
            if (connect()) {
                channel.join<UdpReceiver>(this);
            }
        } else {
            MIDAS_LOG_ERROR("UdpReceiver: " << get_name() << " already started and its state is " << state);
        }
    }

    void async_stop() {
        if (is_closed()) return;

        set_state(NetState::Closed);
        disconnect();
        channel.leave<UdpReceiver>(this);
    }

    void set_socket_options() {
        int recvBufferSize = Config::instance().get<int>(configPath + ".rcvbuf", 0);
        if (!recvBufferSize) {
            recvBufferSize = DefaultReceiveBufferSize;
        }
        skt.set_option(boost::asio::socket_base::receive_buffer_size(recvBufferSize));

        int sendBufferSize = Config::instance().get<int>(configPath + ".sndbuf", 0);
        if (sendBufferSize) {
            skt.set_option(boost::asio::socket_base::send_buffer_size(sendBufferSize));
        }

        bool enableLoopback = Config::instance().get<bool>(configPath + ".enable_loopback", false);
        if (enableLoopback) {
            skt.set_option(boost::asio::ip::multicast::enable_loopback(enableLoopback));
        }

        skt.non_blocking(Config::instance().get<bool>(configPath + ".non_block_io", false));
    }

    // async read from channel main strand for sync
    void async_receive_from(StrandType<true>) {
        skt.async_receive_from(buffer, senderEndpoint,
                               channel.mainStrand.wrap(boost::bind(&UdpReceiver::on_receive_from, SharedPtr(this),
                                                                   boost::asio::placeholders::error,
                                                                   boost::asio::placeholders::bytes_transferred)));
    }

    // async read_some not using sync
    void async_receive_from(StrandType<false>) {
        skt.async_receive_from(buffer, senderEndpoint, boost::bind(&UdpReceiver::on_receive_from, SharedPtr(this),
                                                                   boost::asio::placeholders::error,
                                                                   boost::asio::placeholders::bytes_transferred));
    }

    // async read from channel main strand for sync (non-blocking mode)
    void async_receive_from_non_block(StrandType<true>) {
        static boost::asio::null_buffers nullBuffers;
        skt.async_receive_from(nullBuffers, senderEndpoint,
                               channel.mainStrand.wrap(boost::bind(&UdpReceiver::on_receive_from_non_block,
                                                                   SharedPtr(this), boost::asio::placeholders::error)));
    }

    // async read_some not using sync (non-blocking mode)
    void async_receive_from_non_block(StrandType<false>) {
        static boost::asio::null_buffers nullBuffers;
        skt.async_receive_from(
            nullBuffers, senderEndpoint,
            boost::bind(&UdpReceiver::on_receive_from_non_block, SharedPtr(this), boost::asio::placeholders::error));
    }

public:
    static void data_callback(const TDataCallback& callback) { data_callback() = callback; }
    void data_callback(const TDataCallback& callback, const CustomCallbackTag&) { dataCallback = callback; }
    TDataCallback data_callback(const CustomCallbackTag&) { return dataCallback; }

    static void data_sequence_callback(const TDataSequenceCallback& callback) { data_sequence_callback() = callback; }
    void data_sequence_callback(const TDataSequenceCallback& callback, const CustomCallbackTag&) {
        dataSequenceCallback = callback;
    }
    TDataSequenceCallback data_sequence_callback(const CustomCallbackTag&) { return dataSequenceCallback; }

    static void tap_in_callback(const TDataCallback& callback) { tap_in_callback() = callback; }
    void tap_in_callback(const TDataCallback& callback, const CustomCallbackTag&) { tapInCallback = callback; }
    TDataCallback tap_in_callback(const CustomCallbackTag&) const { return tapInCallback; }

    static void tap_out_callback(const TDataCallback& callback) { tap_out_callback() = callback; }
    void tap_out_callback(const TDataCallback& callback, const CustomCallbackTag&) { tapOutCallback = callback; }
    TDataCallback tap_out_callback(const CustomCallbackTag&) const { return tapOutCallback; }

    // custom header size which will be allocated at the begin of each received buffer
    static size_t& reserve_header_size() {
        static size_t headerSize = 0;
        return headerSize;
    }

    static void default_config_path(const string& p) { default_config_path() = p; }
    static string& default_config_path() {
        static string _path{"midas.net.udp_in"};
        return _path;
    }

private:
    // bind to local interface:service for multicast connection
    void bind_socket_multicast() {
        try {
            if (!skt.is_open()) skt.open(boost::asio::ip::udp::v4());
            set_socket_options();
            skt.set_option(boost::asio::ip::udp::socket::reuse_address(true));

            boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address.ip), address.port);
            skt.bind(endpoint);
            MIDAS_LOG_INFO("socket bind local endpoint: " << skt.local_endpoint());
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error bind socket to " << address.interface << " : " << e.what());
        }
    }

    void bind_socket_unicast() {
        try {
            if (!skt.is_open()) skt.open(boost::asio::ip::udp::v4());
            set_socket_options();

            if (!address.bindIp.empty()) {
                boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address.ip),
                                                        address.port);
                skt.bind(endpoint);
                MIDAS_LOG_INFO("socket bind local endpoint: " << skt.local_endpoint());
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error bind socket to " << address.interface << " : " << e.what());
        }
    }

    void join_multicast() {
        if (!address.bindIp.empty()) {
            skt.set_option(
                boost::asio::ip::multicast::join_group(boost::asio::ip::address_v4::from_string(address.ip),
                                                       boost::asio::ip::address_v4::from_string(address.bindIp)));
            MIDAS_LOG_INFO("join multicast group: " << address.host << " on interface " << address.bindIp);
        } else {
            skt.set_option(
                boost::asio::ip::multicast::join_group(boost::asio::ip::address_v4::from_string(address.ip)));
            MIDAS_LOG_INFO("join multicast group: " << address.host << " on default interface");
        }

        set_state(NetState::Connected);
    }

    void join_unicast() {
        boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address_v4::from_string(address.ip), address.port);
        boost::system::error_code e;
        skt.connect(endpoint, e);
        if (e) {
            MIDAS_LOG_ERROR("failed to open unicast connection to " << endpoint);
            return;
        }

        MIDAS_LOG_INFO("successfully open unicast connection to " << endpoint << ". local endpoint "
                                                                  << skt.local_endpoint());
        set_state(NetState::Connected);
    }

    bool connect() {
        try {
            if (is_multicast()) {
                bind_socket_multicast();
                join_multicast();
            } else {
                bind_socket_unicast();
                join_unicast();
            }

            if (skt.non_blocking()) {
                async_receive_from_non_block(StrandType<UseStrand>());
            } else {
                async_receive_from(StrandType<UseStrand>());
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error starting udp receiver: " << get_name() << " : " << e.what());
            return false;
        }
        return true;
    }

    // leave multicast group and close socket
    void disconnect() {
        try {
            if (is_multicast()) {
                if (!address.bindIp.empty()) {
                    skt.set_option(boost::asio::ip::multicast::leave_group(
                        boost::asio::ip::address_v4::from_string(address.ip),
                        boost::asio::ip::address_v4::from_string(address.bindIp)));
                } else {
                    skt.set_option(
                        boost::asio::ip::multicast::leave_group(boost::asio::ip::address_v4::from_string(address.ip)));
                }
            }
            if (skt.is_open()) skt.close();
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error close socket " << get_name() << " : " << e.what());
        }
    }

    void on_receive_from_non_block(const boost::system::error_code& ec) {
        if (!ec) {
            boost::system::error_code e;
            if (is_likely_hint(enabled)) {
                uint64_t pollingStartTime = (pollingTime ? ntime() : 0);
                uint64_t recvTime = 0;

                bufferSequence.clear();
                do {
                    uint64_t recvStartTime = (receiveTimeout ? ntime() : 0);
                    do {
#ifdef MIDAS_ENABLE_RECVMMSG
#else
                        size_t byteReceived = skt.receive_from(buffer, senderEndpoint, 0, e);
                        if (e == boost::asio::error::would_block) break;  // no more pending msg

                        recvTime = ntime();
                        buffer.time(recvTime);
                        buffer.source(Member::id);
                        buffer.store(byteReceived);
                        bufferSequence.push_back(buffer);

                        // allocate new buffer and let old for client
                        buffer = MutableBuffer(BufferSize, Allocator());
                        if (reserve_header_size()) buffer.store(reserve_header_size());
#endif
                    } while (!receiveTimeout || (recvTime - recvStartTime) < receiveTimeout);

                    if (!bufferSequence.empty()) {
                        // invoke callback
                        dataSequenceCallback(bufferSequence, SharedPtr(this));
                        if (tapInCallback) {
                            for (auto i = bufferSequence.begin(), iEnd = bufferSequence.end(); i != iEnd; ++i) {
                                tapInCallback(*i, SharedPtr(this));
                            }
                        }
                        add_recv_msg_2_stat(bufferSequence.byteSize);

                        bufferSequence.clear();
                        pollingStartTime = recvTime;
                    }
                } while (pollingTime && (ntime() - pollingStartTime) < pollingTime);
            } else {
                while (true) {
                    size_t byteReceived = skt.receive_from(buffer, senderEndpoint, 0, e);
                    if (e == boost::asio::error::would_block) break;  // no more pending msg
                    add_recv_msg_2_stat(bufferSequence.byteSize);
                }
            }
            async_receive_from_non_block(StrandType<UseStrand>());
        } else if (!is_closed()) {
            MIDAS_LOG_ERROR("error recv from " << get_name() << " : " << ec.message());
            if (ec.value() != boost::asio::error::connection_refused) {
                stop();
            } else {
                async_receive_from_non_block(StrandType<UseStrand>());
            }
        }
    }

    void on_receive_from(const boost::system::error_code& ec, size_t byteReceived) {
        if (!ec) {
            if (is_likely_hint(enabled)) {
                buffer.time(ntime());
                buffer.source(Member::id);
                buffer.store(byteReceived);

                size_t byteProcessed = dataCallback(buffer, SharedPtr(this));
                assert(byteProcessed == buffer.size());  // full udp msg must be consumed

                if (tapInCallback) {
                    tapInCallback(buffer, SharedPtr(this));
                }
                // allocate new buffer and let old for client
                buffer = MutableBuffer(BufferSize, Allocator());
                if (reserve_header_size()) buffer.store(reserve_header_size());
            }
            add_recv_msg_2_stat(byteReceived);
            async_receive_from(StrandType<UseStrand>());
        } else if (!is_closed()) {
            MIDAS_LOG_ERROR("error recv from " << get_name() << " : " << ec.message());
            if (ec.value() != boost::asio::error::connection_refused) {
                stop();
            } else {
                async_receive_from(StrandType<UseStrand>());
            }
        }
    }

    void data_sequence_callback_wrap(const ConstBufferSequence& cbs, const TDataCallback& callback) {
        for (auto i = bufferSequence.begin(), iEnd = bufferSequence.end(); i != iEnd; ++i) {
            callback(*i, SharedPtr(this));
        }
    }

    void resolve(UdpAddress& addr) {
        try {
            boost::asio::ip::udp::resolver resolver(channel.iosvc);
            {
                boost::asio::ip::udp::resolver::query q(boost::asio::ip::udp::v4(), addr.host, addr.service);
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

private:
    static bool default_data_callback(const ConstBuffer& msg) { return msg.size(); }
    static TDataCallback& data_callback() {
        static TDataCallback _callback = MIDAS_STATIC_BIND1(&UdpReceiver::default_data_callback);
        return _callback;
    }

    static TDataSequenceCallback& data_sequence_callback() {
        static TDataSequenceCallback _callback;
        return _callback;
    }

    static TDataCallback& tap_in_callback() {
        static TDataCallback _callback;
        return _callback;
    }
    static TDataCallback& tap_out_callback() {
        static TDataCallback _callback;
        return _callback;
    }
};
}

#endif
