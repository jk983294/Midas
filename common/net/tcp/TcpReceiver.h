#ifndef MIDAS_TCP_RECEIVER_H
#define MIDAS_TCP_RECEIVER_H

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>
#include "net/buffer/MutableBuffer.h"
#include "net/channel/Channel.h"
#include "net/common/NetworkHelper.h"
#include "process/admin/AdminHandler.h"

namespace midas {

static const size_t DefaultTcpRecvBufferSize = 256 * 1024;

struct Address {
    string host;
    string service;
    string interface;
    string configPath;
    Address() {}
    Address(const string& h, const string& s, const string& i, const string& cfg)
        : host(h), service(s), interface(i), configPath(cfg) {}
    bool is_valid() const { return (!host.empty() && !service.empty()); }
};

struct Retry {
    int retryTime{0};
    int retryNumber{0};
    int maxRetry{-1};  // default keeping retry
    int maxRetryTime{30};
    Retry() {}
    Retry(string cfg)
        : maxRetry(Config::instance().get<int>(cfg + ".max_retry", -1)),
          maxRetryTime(Config::instance().get<int>(cfg + ".max_retry_time", 30)) {}
    bool expired() const { return (maxRetry != -1 && retryNumber >= maxRetry); }
    void add_retry() {
        ++retryNumber;
        retryTime = (retryTime ? retryTime * 2 : 1);
        if (retryTime > maxRetryTime) retryTime = maxRetryTime;
    }
    void reset() { retryNumber = retryTime = 0; }
};

// object to listen and recv incoming msg
template <typename Tag = DefaultTcpReceiverTag, size_t BufferSize = DefaultTcpRecvBufferSize,
          typename Allocator = PoolAllocator<BufferSize, Tag>, bool UseStrand = false>
class TcpReceiver : public Member {
public:
    template <bool T>
    struct StrandType {
        static const bool value = T;
    };
    typedef typename boost::intrusive_ptr<TcpReceiver> SharedPtr;
    typedef boost::function<bool(SharedPtr)> TConnectCallback;
    typedef boost::function<bool(SharedPtr)> TDisconnectCallback;
    typedef boost::function<bool(SharedPtr)> TRetryCallback;
    typedef boost::function<size_t(const ConstBuffer&, SharedPtr)> TDataCallback;

    Address address[static_cast<size_t>(AddressNumber)];
    Retry retry[static_cast<size_t>(AddressNumber)];
    volatile TcpAddressType addressType;
    CChannel& channel;
    boost::asio::io_service::strand& _strand;
    TConnectCallback connectCallback{connect_callback()};
    TDisconnectCallback disconnectCallback{disconnect_callback()};
    TRetryCallback retryCallback{retry_callback()};
    TDataCallback dataCallback{data_callback()};
    TDataCallback tapInCallback{tap_in_callback()};
    TDataCallback tapOutCallback{tap_out_callback()};
    boost::asio::ip::tcp::socket skt;
    boost::asio::ip::tcp::resolver _resolver;
    boost::asio::ip::tcp::endpoint _endpoint;
    size_t peekBytesRecv{0}, peekBytesSent{0};  // record peak volume
    bool disableAddressSwap{false};
    boost::asio::deadline_timer retryTimer;
    MutableBuffer buffer{BufferSize, Allocator()};
    bool notifyDisconnectAfterStopped;

public:
    TcpReceiver(const Address& primary, const Address& secondary, CChannel& input, bool notify = true)
        : addressType(PrimaryAddress),
          channel(input),
          _strand(input.mainStrand),
          skt(input.iosvc),
          _resolver(input.iosvc),
          retryTimer(input.iosvc),
          notifyDisconnectAfterStopped(notify) {
        address[PrimaryAddress] = primary;
        address[SecondaryAddress] = secondary;
        retry[PrimaryAddress] = Retry(primary.configPath);
        retry[SecondaryAddress] = Retry(secondary.configPath);

        set_name(member_name(primary.host, primary.service), (int)PrimaryAddress);
        set_name(member_name(secondary.host, secondary.service), (int)SecondaryAddress);

        set_description(string("primary:") + member_name(primary.host, primary.service), (int)PrimaryAddress);
        set_description(string("secondary:") + member_name(secondary.host, secondary.service), (int)SecondaryAddress);

        netProtocol = p_tcp_primary;
    }
    TcpReceiver(const Address& primary, const Address& secondary, CChannel& input, boost::asio::io_service::strand& s,
                bool notify = true)
        : addressType(primary),
          channel(input),
          _strand(s),
          skt(input.iosvc),
          _resolver(input.iosvc),
          retryTimer(input.iosvc),
          notifyDisconnectAfterStopped(notify) {
        address[PrimaryAddress] = primary;
        address[SecondaryAddress] = secondary;
        retry[PrimaryAddress] = Retry(primary.configPath);
        retry[SecondaryAddress] = Retry(secondary.configPath);

        set_name(member_name(primary.host, primary.service), (int)PrimaryAddress);
        set_name(member_name(secondary.host, secondary.service), (int)SecondaryAddress);

        set_description(string("primary:") + member_name(primary.host, primary.service), (int)PrimaryAddress);
        set_description(string("secondary:") + member_name(secondary.host, secondary.service), (int)SecondaryAddress);

        netProtocol = p_tcp_primary;
    }
    ~TcpReceiver() {}

    static SharedPtr new_instance(const string& h, const string& s, const string& i, const string& cfg,
                                  const string& sh, const string& ss, const string& si, const string& scfg,
                                  CChannel& input, bool startReceiver = true, bool notify = true) {
        Address primary(h, s, !i.empty() ? i : Config::instance().get<string>(cfg + ".bind_address", ""), cfg);
        Address secondary(sh, ss, !si.empty() ? si : Config::instance().get<string>(scfg + ".bind_address", ""), scfg);
        SharedPtr receiver(new TcpReceiver(primary, secondary, input, notify));
        if (startReceiver) receiver->start();
        return receiver;
    }

    static SharedPtr new_instance(const string& h, const string& s, const string& i, const string& cfg,
                                  const string& sh, const string& ss, const string& si, const string& scfg,
                                  CChannel& input, boost::asio::io_service::strand& strand_, bool startReceiver = true,
                                  bool notify = true) {
        Address primary(h, s, !i.empty() ? i : Config::instance().get<string>(cfg + ".bind_address", ""), cfg);
        Address secondary(sh, ss, !si.empty() ? si : Config::instance().get<string>(scfg + ".bind_address", ""), scfg);
        SharedPtr receiver(new TcpReceiver(primary, secondary, input, strand_, notify));
        if (startReceiver) receiver->start();
        return receiver;
    }

    static SharedPtr new_instance(const string& h, const string& s, const string& i, const string& cfg, CChannel& input,
                                  bool startReceiver = true, bool notify = true) {
        return new_instance(h, s, i, cfg, "", "", "", "", input, startReceiver, notify);
    }

    static SharedPtr new_instance(const string& h, const string& s, const string& i, const string& cfg, CChannel& input,
                                  boost::asio::io_service::strand& strand_, bool startReceiver = true,
                                  bool notify = true) {
        return new_instance(h, s, i, cfg, "", "", "", "", input, strand_, startReceiver, notify);
    }

    static SharedPtr new_instance(const string& h, const string& s, const string& i, CChannel& input,
                                  boost::asio::io_service::strand& strand_, bool startReceiver = true,
                                  bool notify = true) {
        return new_instance(h, s, i, default_config_path(), input, strand_, startReceiver, notify);
    }

    static SharedPtr new_instance(const string& h, const string& s, const string& i, CChannel& input,
                                  bool startReceiver = true, bool notify = true) {
        return new_instance(h, s, i, default_config_path(), input, startReceiver, notify);
    }

    static SharedPtr new_instance(const string& h, const string& s, CChannel& input, bool startReceiver = true,
                                  bool notify = true) {
        return new_instance(h, s, "", input, startReceiver, notify);
    }

    static SharedPtr new_instance(const string& h, const string& s, CChannel& input,
                                  boost::asio::io_service::strand& strand_, bool startReceiver = true,
                                  bool notify = true) {
        return new_instance(h, s, "", input, strand_, startReceiver, notify);
    }

    static string member_name(const string& h, const string& s) { return h + ":" + s; }

    // create new receiver as handler of admin command
    static string admin_new_receiver(const string& cmd, const TAdminCallbackArgs& args, CChannel& input,
                                     const string& group, const string& cfg) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string data = args[0];
        string host, service, interface;

        split_host_port_interface(data, host, service, interface);

        // desc and config path
        string s1 = (args.size() > 1 ? args[1] : "");
        string s2 = (args.size() > 2 ? args[2] : "");
        string description(group);
        string adminCfgPath = (cfg.empty() ? default_config_path() : cfg);

        // find backup connection
        string sData;
        string shost, sservice, sinterface, ss1, ss2;
        if (s1.find(':') == std::string::npos) {
            sData = s1;
            ss1 = s2;
            ss2 = (args.size() > 3 ? args[3] : "");
        } else if (s2.find(':') == std::string::npos) {
            sData = s2;
            ss1 = (args.size() > 3 ? args[3] : "");
            ss2 = (args.size() > 4 ? args[4] : "");
        } else {
            sData = (args.size() > 3 ? args[3] : "");
            ss1 = (args.size() > 4 ? args[4] : "");
            ss2 = (args.size() > 5 ? args[5] : "");
        }
        if (!sData.empty()) {
            split_host_port_interface(sData, shost, sservice, sinterface);
        }
        string sAdminCfgPath = (cfg.empty() ? default_config_path() : cfg);

        if (input.get_member(member_name(host, service))) return string("connection alread exist!");
        MIDAS_LOG_INFO("connect to " << host << ":" << service
                                     << " interface: " << (!interface.empty() ? interface : "default"));
        SharedPtr receiver = new_instance(host, service, interface, adminCfgPath, shost, sservice, sinterface,
                                          sAdminCfgPath, input, false);
        if (receiver) {
            receiver->set_description(description);
            receiver->start();
            return string("new connection success");
        }
        return string("failed to open connection");
    }

    static string admin_stop_receiver(const string& cmd, const TAdminCallbackArgs& args, CChannel& input) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string name = args[0];
        MemberPtr connection = input.get_member(name);
        if (!connection) return "connection not found.";
        TcpReceiver* receiver = dynamic_cast<TcpReceiver*>(connection.get());
        if (receiver) receiver->stop();
        return "connection closed";
    }

    static void register_admin(CChannel& input, AdminHandler& handler, const string& group = string(),
                               const string& cfg = string()) {
        string open_cmd{"open_server"};
        string close_cmd{"close_server"};
        if (!group.empty()) {
            open_cmd += "_" + group;
            close_cmd += "_" + group;
        }

        handler.register_callback(open_cmd,
                                  boost::bind(&TcpReceiver::admin_new_receiver, _1, _2, boost::ref(input), group, cfg),
                                  "open connection", "usage: host:port[:interface] [cfg]");
        handler.register_callback(close_cmd, boost::bind(&TcpReceiver::admin_stop_receiver, _1, _2, boost::ref(input)),
                                  input, "close connection", "usage: host:port");
    }

    void start() {
        channel.work(1);
        if (UseStrand)
            async_start();
        else
            starting();
    }

    void stop() {
        if (UseStrand)
            async_stop();
        else
            stopping();
    }

    // force disconnect and let retry handler to rebuild connection
    void restart() {
        if (UseStrand)
            async_restart();
        else
            restarting();
    }

    void enable() {
        if (state == Connected) {
            MIDAS_LOG_INFO("restart tcp connection to re-enable it");
            restart();
        }
        Member::enable();
    }

    // deliver msg to upstream
    void deliver(const ConstBuffer& msg) { send(msg); }

    // send msg to upstream
    void send(const ConstBuffer& msg) {
        if (!skt.is_open()) return;

        try {
            if (boost::asio::write(skt, msg) != msg.size()) {
                MIDAS_LOG_ERROR("error write to socket " << get_name());
            } else {
                if (tapOutCallback) tapOutCallback(msg, SharedPtr(this));
                add_sent_msg_2_stat(msg.size());
                if (msg.size() > peekBytesSent) peekBytesSent = msg.size();
            }
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error write to socket " << get_name() << " : " << e.what());
        }
    }

    void stats(ostream& os) {
        Member::stats(os);

        if (skt.is_open()) {
            os << "address  " << skt.remote_endpoint() << '\n' << "local  " << skt.local_endpoint() << '\n';
        }
        os << "peek in  " << peekBytesRecv << '\n' << "peek out  " << peekBytesSent << '\n';
    }

public:
    static void connect_callback(const TConnectCallback& callback) { connect_callback() = callback; }
    void connect_callback(const TConnectCallback& callback, const CustomCallbackTag&) { connectCallback = callback; }
    static void disconnect_callback(const TDisconnectCallback& callback) { disconnect_callback() = callback; }
    void disconnect_callback(const TDisconnectCallback& callback, const CustomCallbackTag&) {
        disconnectCallback = callback;
    }
    static void retry_callback(const TRetryCallback& callback) { retry_callback() = callback; }
    void retry_callback(const TRetryCallback& callback, const CustomCallbackTag&) { retryCallback = callback; }
    static void data_callback(const TDataCallback& callback) { data_callback() = callback; }
    void data_callback(const TDataCallback& callback, const CustomCallbackTag&) { dataCallback = callback; }
    static void tap_in_callback(const TDataCallback& callback) { tap_in_callback() = callback; }
    void tap_in_callback(const TDataCallback& callback, const CustomCallbackTag&) { tapInCallback = callback; }
    TDataCallback tap_in_callback(const CustomCallbackTag&) const { return tapInCallback; }
    static void tap_out_callback(const TDataCallback& callback) { tap_out_callback() = callback; }
    void tap_out_callback(const TDataCallback& callback, const CustomCallbackTag&) { tapOutCallback = callback; }
    TDataCallback tap_out_callback(const CustomCallbackTag&) const { return tapOutCallback; }
    static void default_config_path(const string& p) { default_config_path() = p; }
    static string& default_config_path() {
        static string _path{"midas.net.tcp_in"};
        return _path;
    }

private:
    void async_start() {
        if (!UseStrand)
            channel.iosvc.dispatch(boost::bind(&TcpReceiver::starting, SharedPtr(this)));
        else
            _strand.dispatch(boost::bind(&TcpReceiver::starting, SharedPtr(this)));
    }

    void starting() {
        if (state != Created) return;
        channel.join<TcpReceiver>(this);
        start_connect();
    }

    void async_stop() {
        if (!UseStrand)
            channel.iosvc.dispatch(boost::bind(&TcpReceiver::stopping, SharedPtr(this)));
        else
            _strand.dispatch(boost::bind(&TcpReceiver::stopping, SharedPtr(this)));
    }

    void stopping() {
        if (state == Created) return;
        state = Created;
        try {
            if (skt.is_open()) {
                skt.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                skt.close();
            }
            _resolver.cancel();
            retryTimer.cancel();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error close socket " << get_name() << " : " << e.what());
        }
        channel.leave<TcpReceiver>(this);
    }

    void async_restart() {
        if (!UseStrand)
            channel.iosvc.dispatch(boost::bind(&TcpReceiver::restarting, SharedPtr(this)));
        else
            _strand.dispatch(boost::bind(&TcpReceiver::restarting, SharedPtr(this)));
    }

    void restarting() {
        disableAddressSwap = true;  // restart on the same address
        try {
            if (skt.is_open()) skt.close();
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error close socket " << get_name() << " : " << e.what());
        }
    }

    void async_connect(boost::asio::ip::tcp::endpoint e, boost::asio::ip::tcp::resolver::iterator e_iterator) {
        if (!UseStrand)
            skt.async_connect(e, boost::bind(&TcpReceiver::on_connect, SharedPtr(this),
                                             boost::asio::placeholders::error, e_iterator));
        else
            skt.async_connect(e, _strand.wrap(boost::bind(&TcpReceiver::on_connect, SharedPtr(this),
                                                          boost::asio::placeholders::error, e_iterator)));
    }

    void async_read_some(StrandType<true>) {
        skt.async_read_some(
            buffer, _strand.wrap(boost::bind(&TcpReceiver::on_read, SharedPtr(this), boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred)));
    }

    void async_read_some(StrandType<false>) {
        skt.async_read_some(buffer,
                            boost::bind(&TcpReceiver::on_read, SharedPtr(this), boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
    }

    void bind_socket() {
        if (!address[static_cast<size_t>(addressType)].interface.empty() &&
            address[static_cast<size_t>(addressType)].host != "localhost" &&
            address[static_cast<size_t>(addressType)].host != "127.0.0.1") {
            typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
            boost::char_separator<char> sep(":");
            tokenizer tok(address[static_cast<size_t>(addressType)].interface, sep);
            auto itr = tok.begin();
            string bindHost, bindService = "0";
            if (itr != tok.end()) bindHost = *itr++;
            if (itr != tok.end()) bindService = *itr;

            if (!bindHost.empty() && ::isalpha(bindHost[0])) {
                try {
                    string ifAddress;
                    NetworkHelper helper;
                    helper.lookup_interface(bindHost, ifAddress);
                    bindHost = ifAddress;
                } catch (const MidasException& e) {
                    MIDAS_LOG_ERROR("error for interface lookup: " << bindHost << " : " << e.what());
                }
            }

            try {
                boost::asio::ip::tcp::resolver::query q(bindHost, bindService);
                boost::asio::ip::tcp::endpoint endpoint = *_resolver.resolve(q);
                if (!skt.is_open()) skt.open(boost::asio::ip::tcp::v4());
                skt.set_option(boost::asio::socket_base::reuse_address(true));
                int32_t lingerTimeout = Config::instance().get<int32_t>(
                    address[static_cast<size_t>(addressType)].configPath + ".linger_time_out", -1);
                if (lingerTimeout >= 0) {
                    boost::asio::socket_base::linger opt(true, lingerTimeout);
                    skt.set_option(opt);
                    MIDAS_LOG_INFO("enable linger time out " << lingerTimeout);
                }
                skt.bind(endpoint);
                MIDAS_LOG_INFO("socket bind success. local endpoint: " << skt.local_endpoint());
            } catch (const boost::system::system_error& e) {
                MIDAS_LOG_ERROR("error bind socket to " << address[static_cast<size_t>(addressType)].interface << " : "
                                                        << e.what());
            }
        }
    }

    void start_connect(const boost::system::error_code& e) {
        if (!e) {
            start_connect();
        }
    }

    void start_connect() {
        if (state != Created && state != Pending) return;
        state = Pending;
        boost::asio::ip::tcp::resolver::query q(address[static_cast<size_t>(addressType)].host,
                                                address[static_cast<size_t>(addressType)].service);
        if (!UseStrand) {
            _resolver.async_resolve(
                q, boost::bind(&TcpReceiver::on_resolve, SharedPtr(this), boost::asio::placeholders::error,
                               boost::asio::placeholders::iterator));
        } else {
            _resolver.async_resolve(
                q, _strand.wrap(boost::bind(&TcpReceiver::on_resolve, SharedPtr(this), boost::asio::placeholders::error,
                                            boost::asio::placeholders::iterator)));
        }
    }

    void retry_connect() {
        state = Pending;
        if (!retryCallback(this)) {
            MIDAS_LOG_INFO("retry callback failed for connection " << get_name());
            stopping();
            return;
        }
        if (skt.is_open()) skt.close();
        if (!disableAddressSwap) {
            TcpAddressType nextAddress(addressType == PrimaryAddress ? SecondaryAddress : PrimaryAddress);

            if (address[(int)nextAddress].is_valid() && !retry[(int)nextAddress].expired()) {
                addressType = nextAddress;
                netProtocol = addressType == PrimaryAddress ? p_tcp_primary : p_tcp_secondary;
                use_name((int)addressType);
                use_description((int)addressType);
            }
        }
        disableAddressSwap = false;

        Retry& tmpRetry = retry[static_cast<size_t>(addressType)];
        if (tmpRetry.expired()) {
            MIDAS_LOG_ERROR("max retry number reached for connection " << get_name() << " giving up...");
            stopping();
            return;
        }
        tmpRetry.add_retry();
        retryTimer.expires_from_now(boost::posix_time::seconds(tmpRetry.retryTime));
        if (!UseStrand)
            retryTimer.async_wait(MIDAS_BIND1(&TcpReceiver::start_connect, SharedPtr(this)));
        else
            retryTimer.async_wait(_strand.wrap(MIDAS_BIND1(&TcpReceiver::start_connect, SharedPtr(this))));
    }

    // called when connection establish
    void connected() {
        try {
            state = Connected;
            int readBufSize =
                Config::instance().get<int>(address[static_cast<size_t>(addressType)].configPath + ".rcvbuf", 0);
            if (!readBufSize) readBufSize = 2 * BufferSize;
            skt.set_option(boost::asio::socket_base::receive_buffer_size(readBufSize));

            int writeBufSize =
                Config::instance().get<int>(address[static_cast<size_t>(addressType)].configPath + ".sndbuf", 0);
            if (writeBufSize) skt.set_option(boost::asio::socket_base::send_buffer_size(writeBufSize));

            {
                // enable keep alive
                bool isKeepAlive = Config::instance().get<bool>(
                    address[static_cast<size_t>(addressType)].configPath + ".keepalive", true);
                boost::system::error_code ec;
                skt.set_option(boost::asio::socket_base::keep_alive(isKeepAlive), ec);
                if (ec) MIDAS_LOG_ERROR("error turn on keep alive option for " << get_name());
            }

            {
                // enable tcp no delay
                bool isNoDelay = Config::instance().get<bool>(
                    address[static_cast<size_t>(addressType)].configPath + ".nodelay", true);
                boost::system::error_code ec;
                skt.set_option(boost::asio::ip::tcp::no_delay(isNoDelay), ec);
                if (ec) MIDAS_LOG_ERROR("error turn on no delay option for " << get_name());
            }

            if (!connectCallback(this)) {
                stopping();
                return;
            }
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error change socket options" << get_name() << " : " << e.what());
        }
        retry[static_cast<size_t>(addressType)].reset();
    }

    void disconnected() {
        if (state == Created) {
            if (notifyDisconnectAfterStopped) disconnectCallback(this);
        } else {
            state = Disconnected;
            if (disconnectCallback(this))
                retry_connect();
            else
                stopping();
        }
    }

    void on_resolve(const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator e_iterator) {
        if (state != Pending) return;
        if (!e) {
            _endpoint = *e_iterator;
            bind_socket();
            async_connect(_endpoint, ++e_iterator);
        } else {
            MIDAS_LOG_ERROR("error cannot resolve the address " << e.message() << " for " << get_name());
        }
    }

    void on_connect(const boost::system::error_code& e, boost::asio::ip::tcp::resolver::iterator e_iterator) {
        if (state != Pending) return;
        if (!e) {
            connected();
            buffer.clear();
            async_read_some(StrandType<UseStrand>());
        } else {
            if (e_iterator != boost::asio::ip::tcp::resolver::iterator()) {
                if (skt.is_open()) skt.close();
                _endpoint = *e_iterator;
                bind_socket();
                async_connect(_endpoint, ++e_iterator);
            } else {
                MIDAS_LOG_ERROR("error no more endpoint to try connect to " << e.message() << " for " << get_name());
                retry_connect();
            }
        }
    }

    void on_read(const boost::system::error_code& e, size_t transferred) {
        if (!e && state == Connected) {
            if (is_likely_hint(enabled)) {
                buffer.time(ntime());
                buffer.source(Member::id);
                buffer.store(transferred);
                size_t processed = dataCallback(buffer, SharedPtr(this));
                if (tapInCallback) tapInCallback(ConstBuffer(buffer, processed), SharedPtr(this));
                // allocate new buffer, let old one for client
                buffer = MutableBuffer(buffer, processed);

                add_recv_msg_2_stat(processed);
                if (transferred > peekBytesRecv) peekBytesRecv = transferred;
            } else {
                add_recv_msg_2_stat(transferred);
            }
            async_read_some(StrandType<UseStrand>());
        } else {
            MIDAS_LOG_ERROR("error cannot read from socket: " << e.message() << " name " << get_name()
                                                              << " transferred: " << transferred);
            disconnected();
        }
    }

private:
    static bool default_connect_callback() { return true; }
    static TConnectCallback& connect_callback() {
        static TConnectCallback _callback = MIDAS_STATIC_BIND0(&TcpReceiver::default_connect_callback);
        return _callback;
    }
    static bool default_disconnect_callback() { return true; }
    static TDisconnectCallback& disconnect_callback() {
        static TDisconnectCallback _callback = MIDAS_STATIC_BIND0(&TcpReceiver::default_disconnect_callback);
        return _callback;
    }
    static bool default_retry_callback() { return true; }
    static TRetryCallback& retry_callback() {
        static TRetryCallback _callback = MIDAS_STATIC_BIND0(&TcpReceiver::default_retry_callback);
        return _callback;
    }
    static bool default_data_callback(const ConstBuffer& msg) { return msg.size(); }
    static TDataCallback& data_callback() {
        static TDataCallback _callback = MIDAS_STATIC_BIND1(&TcpReceiver::default_data_callback);
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
