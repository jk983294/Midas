#ifndef MIDAS_TCP_RECV_GROUP_H
#define MIDAS_TCP_RECV_GROUP_H

#include <tbb/atomic.h>
#include <tbb/concurrent_vector.h>
#include <unistd.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/functional.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>
#include <cstring>
#include <ctime>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include "net/buffer/ConstBufferWriter.h"
#include "net/buffer/MutableBuffer.h"
#include "net/channel/Channel.h"
#include "net/common/Member.h"
#include "net/common/NetworkHelper.h"
#include "net/common/TimerMember.h"
#include "process/admin/AdminHandler.h"
#include "utils/ConvertHelper.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

/**
 * TcpRecvGroup owns a group of TcpRecv instances. Each TcpRecv owns a tcp connection.
 * TcpRecvGroup is able to accept tcp connection as long as it created with port
 * TcpRecvGroup has sole ownership of its TcpRecv instances.
 * TcpRecv is returned as a pointer or a reference rather than a shared ptr.
 * returning false in accept_callback makes TcpRecvGroup disconnect the TcpRecv from its peer. and TcpRecv
 * instance is not connectable in this case
 */
class TcpRecvGroup : public Member, private boost::noncopyable {
public:
    /**
     * TcpRecv owns a tcp connection. TcpRecv is created by either invoking new_tcp_recv method
     * or its TcpRecvGroup instance accept a tcp connection initiated by a peer.
     * in first case, isConnectable is true
     * in second case, the TcpRecv instance is connected and isConnectable is false
     * returning false in accept_callback makes TcpRecv disconnect from its peer.
     * returning false in disconnect_callback makes TcpRecv not retry to connect to its peer.
     * TcpRecv invoke retry_callback every time before it retries to connect.
     * returning false in retry_callback stops TcpRecv from retrying to connect.
     */
    class TcpRecv : public Member, private boost::noncopyable {
    public:
        typedef boost::intrusive_ptr<TcpRecv> SharedPtr;
        typedef boost::function<bool(TcpRecv&)> TConnectCallback;
        typedef boost::function<bool(TcpRecv&)> TDisconnectCallback;
        typedef boost::function<size_t(const ConstBuffer&, TcpRecv&)> TDataCallback;
        typedef boost::function<bool(TcpRecv&)> TRetryCallback;

        bool running{false};
        TcpRecvGroup& group_;
        const bool isConnectable;
        boost::scoped_ptr<boost::asio::io_service::strand> strandPtr;
        boost::asio::io_service::strand* strand_;
        boost::asio::deadline_timer timer_;
        vector<TcpAddress> addrVec;
        size_t tcpAddrIndex_{0};
        const string cfg;
        boost::asio::ip::tcp::socket skt;
        boost::asio::ip::tcp::resolver resolver_;
        boost::asio::ip::tcp::endpoint localEndpoint;
        boost::asio::ip::tcp::endpoint remoteEndpoint;
        MutableBuffer mutableBuffer;
        ConstBufferWriter bufferWriter;
        TConnectCallback connectCallback;
        TDisconnectCallback disconnectCallback;
        TDataCallback dataCallback;
        TRetryCallback retryCallback;
        size_t retryCount{0};
        boost::posix_time::time_duration retryTime;
        const size_t maxRetryCount;
        const boost::posix_time::time_duration maxRetryTime;

    public:
        template <typename Allocator>
        TcpRecv(TcpRecvGroup& group1, const vector<TcpAddress>& addrList, const string& cfg_,
                boost::asio::io_service& iosvc, boost::asio::io_service::strand* strand, size_t bs,
                const Allocator allocator)
            : group_(group1),
              isConnectable(!addrList.empty()),
              strandPtr(strand ? nullptr : new boost::asio::io_service::strand(iosvc)),
              strand_(strand ? strand : strandPtr.get()),
              timer_(iosvc),
              addrVec(addrList),
              cfg(cfg_),
              skt(iosvc),
              resolver_(iosvc),
              mutableBuffer(bs, allocator),
              bufferWriter(4096),
              connectCallback(connect_callback()),
              disconnectCallback(disconnect_callback()),
              dataCallback(data_callback()),
              retryCallback(retry_callback()),
              retryTime(boost::posix_time::not_a_date_time),
              maxRetryCount(
                  Config::instance().get<size_t>(this->cfgPath() + ".max_retry", std::numeric_limits<size_t>::max())),
              maxRetryTime(Config::instance().get<boost::posix_time::time_duration>(this->cfgPath() + ".max_retry_time",
                                                                                    boost::posix_time::seconds(30))) {
            if (!addrVec.empty()) {
                set_name(false);
            }
            netProtocol = NetProtocol::p_tcp_primary;
            Member::displayStat = true;
        }

        bool is_connectable() const { return isConnectable; }

        const boost::asio::io_service::strand& strand() const { return *strand_; }

        /**
         * no side effect if TcpRecv is not connectable
         */
        void connect() {
            if (!isConnectable) return;
            strand_->post(boost::bind(&TcpRecv::async_start, this));
        }

        /**
         * disconnect if it is connected and stop it from retrying to connect when is is connectable
         */
        void disconnect() { strand_->post(boost::bind(&TcpRecv::async_stop, this)); }

        void deliver(const ConstBuffer& msg) { strand_->post(boost::bind(&TcpRecv::async_write, this, msg)); }

        static void connect_callback(const TConnectCallback& ccb) { connect_callback() = ccb; }

        void connect_callback(const TConnectCallback& ccb, const CustomCallbackTag&) { connectCallback = ccb; }

        static void disconnect_callback(const TDisconnectCallback& dcb) { disconnect_callback() = dcb; }

        void disconnect_callback(const TDisconnectCallback& dcb, const CustomCallbackTag&) { disconnectCallback = dcb; }

        static void data_callback(const TDataCallback& dcb) { data_callback() = dcb; }

        void data_callback(const TDataCallback& dcb, const CustomCallbackTag&) { dataCallback = dcb; }

        static void retry_callback(const TRetryCallback& rcb) { retry_callback() = rcb; }

        void retry_callback(const TRetryCallback& rcb, const CustomCallbackTag&) { retryCallback = rcb; }

        template <typename Stream>
        void print(Stream& s) const {
            s << " tcp recv[ name[" << Member::get_name() << "] desc[" << Member::get_description() << "] is running["
              << get_bool_string(running) << "] is connectable[" << get_bool_string(isConnectable) << "] state["
              << Member::state << "]";
            std::for_each(addrVec.begin(), addrVec.end(), boost::bind(&TcpAddress::print<Stream>, _1, boost::ref(s)));
            s << " local endpoint[" << localEndpoint << "] remote endpoint[" << remoteEndpoint << "]]";
        }

        void async_start() {
            if (running) return;
            running = true;
            MIDAS_LOG_INFO("this tcp recv started (" << *this << ").");
            if (isConnectable) {
                assert(!skt.is_open());
                group_.channel.join<TcpRecv>(TcpRecv::SharedPtr(this));
                resolve(0, false);
            } else {
                assert(skt.is_open());
                group_.channel.join<TcpRecv>(TcpRecv::SharedPtr(this));
                if (connectCallback(*this)) {
                    set_state(NetState::Connected);
                    deliver(ConstBuffer());
                    skt.async_read_some(mutableBuffer, strand_->wrap(boost::bind(
                                                           &TcpRecv::on_read, this, boost::asio::placeholders::error,
                                                           boost::asio::placeholders::bytes_transferred)));
                } else {
                    strand_->post(boost::bind(&TcpRecv::async_stop, this));
                }
            }
        }

        void async_stop() {
            if (!running) return;
            boost::system::error_code e;
            timer_.cancel(e);
            reset();
            localEndpoint = boost::asio::ip::tcp::endpoint();
            remoteEndpoint = boost::asio::ip::tcp::endpoint();
            mutableBuffer.clear();
            bufferWriter.reset();

            if (Member::state == NetState::Connected) {
                disconnectCallback(*this);
            }

            group_.channel.leave<TcpRecv>(TcpRecv::SharedPtr(this));
            MIDAS_LOG_INFO("this tcp recv is stopped. " << *this)
            running = false;
        }

        void async_resolve(size_t tcpAddrIndex, const boost::system::error_code& e) {
            if (!running) return;
            if (e) {
                MIDAS_LOG_ERROR("this tcp recv failed to resolve " << e << *this);
                return;
            }

            boost::asio::ip::tcp::resolver::query q(boost::asio::ip::tcp::v4(), addrVec[tcpAddrIndex].host,
                                                    addrVec[tcpAddrIndex].service);
            resolver_.async_resolve(
                q, boost::bind(&TcpRecv::on_resolve, this, tcpAddrIndex, boost::asio::placeholders::error,
                               boost::asio::placeholders::iterator));
        }

        void resolve(size_t tcpAddrIndex, bool retry) {
            Member::use_name(tcpAddrIndex);
            localEndpoint = boost::asio::ip::tcp::endpoint();
            remoteEndpoint = boost::asio::ip::tcp::endpoint();
            boost::system::error_code e;
            timer_.cancel(e);
            if (e) {
                MIDAS_LOG_ERROR("failed to cancel its timer " << e << *this);
                return;
            }

            boost::system::error_code e1;
            if (retry) {
                ++retryCount;
                if (retryCount > maxRetryCount || !retryCallback) {
                    retryCount = 0;
                    retryTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time);
                    return;
                }
                retryTime = retryTime.is_special() ? boost::posix_time::seconds(1) : (retryTime * 2);
                if (retryTime < maxRetryTime) {
                    retryTime = maxRetryTime;
                }
                timer_.expires_from_now(retryTime, e1);
            } else {
                timer_.expires_from_now(boost::posix_time::seconds(0), e1);
            }

            if (e1) {
                MIDAS_LOG_ERROR("failed to expire its timer " << e1 << *this);
                return;
            }

            timer_.async_wait(strand_->wrap(boost::bind(&TcpRecv::async_resolve, this, tcpAddrIndex, _1)));
        }

        void on_resolve(size_t tcpAddrIndex, const boost::system::error_code& e,
                        boost::asio::ip::tcp::resolver::iterator it) {
            if (!running) return;
            if (e) {
                MIDAS_LOG_ERROR("failed to resolve " << e << *this);
                if (boost::asio::error::operation_aborted == e) return;
                resolve((1 + tcpAddrIndex) % addrVec.size(), true);
                return;
            }

            boost::system::error_code e1;
            skt.open(boost::asio::ip::tcp::v4(), e1);
            if (e1) {
                MIDAS_LOG_ERROR("failed to open socket " << e1 << *this);
                resolve((1 + tcpAddrIndex) % addrVec.size(), true);
                return;
            }

            MIDAS_LOG_INFO("this tcp recv is resolved " << *this);
            bind_socket(tcpAddrIndex);
            remoteEndpoint = *it;
            skt.async_connect(remoteEndpoint, strand_->wrap(boost::bind(&TcpRecv::on_connect, this, tcpAddrIndex,
                                                                        boost::asio::placeholders::error, ++it)));
        }

        void on_connect(size_t tcpAddrIndex, const boost::system::error_code& e,
                        boost::asio::ip::tcp::resolver::iterator it) {
            if (!running) return;
            if (e) {
                MIDAS_LOG_ERROR("failed to connect " << e << *this);
                if (boost::asio::error::operation_aborted == e) return;
                if (boost::asio::ip::tcp::resolver::iterator() == it) {
                    reset();
                    resolve((1 + tcpAddrIndex) % addrVec.size(), true);
                    return;
                }

                remoteEndpoint = *it;
                skt.async_connect(remoteEndpoint, strand_->wrap(boost::bind(&TcpRecv::on_connect, this, tcpAddrIndex,
                                                                            boost::asio::placeholders::error, ++it)));
                return;
            }
            MIDAS_LOG_INFO("this tcp recv is connected " << *this);
            boost::system::error_code e1;
            localEndpoint = skt.local_endpoint(e1);
            if (e1) {
                MIDAS_LOG_ERROR("failed to retrieve its local endpoint " << e1 << *this);
            }

            if (connectCallback(*this)) {
                set_state(NetState::Connected);
                retryCount = 0;
                retryTime = boost::posix_time::time_duration(boost::posix_time::not_a_date_time);
                set_socket_option();
                deliver(ConstBuffer());
                skt.async_read_some(mutableBuffer,
                                    strand_->wrap(boost::bind(&TcpRecv::on_read, this, boost::asio::placeholders::error,
                                                              boost::asio::placeholders::bytes_transferred)));
            } else {
                strand_->post(boost::bind(&TcpRecv::async_stop, this));
            }
        }

        void on_read(const boost::system::error_code& e, size_t transferred) {
            if (!running) return;
            if (e) {
                MIDAS_LOG_ERROR("failed to read " << e << *this);
                if (boost::asio::error::operation_aborted == e || Member::state != NetState::Connected) return;
                reset();
                localEndpoint = boost::asio::ip::tcp::endpoint();
                remoteEndpoint = boost::asio::ip::tcp::endpoint();
                mutableBuffer.clear();
                bufferWriter.reset();

                if (isConnectable && disconnectCallback(*this)) {
                    resolve(0, true);
                } else {
                    group_.channel.leave<TcpRecv>(TcpRecv::SharedPtr(this));
                    running = false;
                }
                return;
            }

            mutableBuffer.time(ntime());
            mutableBuffer.source(Member::id);
            mutableBuffer.store(transferred);

            Member::add_recv_msg_2_stat(transferred);
            size_t consumed = dataCallback(mutableBuffer, *this);
            mutableBuffer = MutableBuffer(mutableBuffer, consumed);
            skt.async_read_some(mutableBuffer,
                                strand_->wrap(boost::bind(&TcpRecv::on_read, this, boost::asio::placeholders::error,
                                                          boost::asio::placeholders::bytes_transferred)));
        }

        void async_write(const ConstBuffer& msg) {
            if (!running) return;
            if (!msg.empty()) {
                bufferWriter.store(msg);
            } else if (bufferWriter.empty()) {
                return;
            }

            if (bufferWriter.is_writing()) return;

            boost::asio::async_write(
                skt, bufferWriter.begin_writing(),
                strand_->wrap(boost::bind(&TcpRecv::on_write, this, boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred)));
        }

        void on_write(const boost::system::error_code& e, size_t transferred) {
            if (!running) {
                bufferWriter.undo_writing();
                return;
            }

            if (e) {
                MIDAS_LOG_ERROR("failed to write " << e << *this);
                if (boost::asio::error::operation_aborted == e || Member::state != NetState::Connected) {
                    bufferWriter.undo_writing();
                    return;
                }
                reset();
                localEndpoint = boost::asio::ip::tcp::endpoint();
                remoteEndpoint = boost::asio::ip::tcp::endpoint();
                mutableBuffer.clear();
                bufferWriter.reset();

                if (isConnectable && disconnectCallback(*this)) {
                    resolve(0, true);
                } else {
                    group_.channel.leave<TcpRecv>(TcpRecv::SharedPtr(this));
                    running = false;
                }
                return;
            }

            bufferWriter.end_writing();
            Member::add_sent_msg_2_stat(transferred);
            if (bufferWriter.more_writing()) {
                boost::asio::async_write(
                    skt, bufferWriter.begin_writing(),
                    strand_->wrap(boost::bind(&TcpRecv::on_write, this, boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred)));
            }
        }

        void bind_socket(size_t tcpAddrIndex) {
            const TcpAddress& tcpAddress = addrVec[tcpAddrIndex];
            if (tcpAddress.itf.empty() || is_localhost(tcpAddress.host)) return;

            string host, service{"0"}, itf;
            split_host_port_interface(tcpAddress.itf, host, service, itf);
            if (!host.empty() && isalpha(host[0])) {
                try {
                    NetworkHelper helper;
                    string addr;
                    helper.lookup_interface(host, addr);
                    host = addr;
                } catch (const MidasException& e) {
                    MIDAS_LOG_ERROR("failed to lookup interface " << e.what() << *this);
                    return;
                }

                boost::system::error_code e1;
                localEndpoint = *resolver_.resolve(boost::asio::ip::tcp::resolver::query(host, service), e1);
                if (e1) {
                    MIDAS_LOG_ERROR("failed to resolve local endpoint " << e1 << *this);
                    return;
                }

                boost::system::error_code e2;
                skt.bind(localEndpoint, e2);
                if (e1) {
                    MIDAS_LOG_ERROR("failed to bind socket " << e2 << *this);
                    return;
                }
                MIDAS_LOG_INFO("this tcp recv binded " << *this);
            }
        }

        void set_socket_option() {
            bool broadcast = Config::instance().get<bool>(cfgPath() + ".broadcast", false);
            boost::system::error_code e1;
            skt.set_option(boost::asio::socket_base::broadcast(broadcast), e1);
            if (e1) {
                MIDAS_LOG_ERROR("failed to set broadcast socket option " << e1 << *this);
            }

            bool doNotRoute = Config::instance().get<bool>(cfgPath() + ".doNotRoute", false);
            boost::system::error_code e2;
            skt.set_option(boost::asio::socket_base::do_not_route(doNotRoute), e2);
            if (e2) {
                MIDAS_LOG_ERROR("failed to set do_not_route socket option " << e2 << *this);
            }

            bool keepalive = Config::instance().get<bool>(cfgPath() + ".keepalive", true);
            boost::system::error_code e3;
            skt.set_option(boost::asio::socket_base::keep_alive(keepalive), e3);
            if (e3) {
                MIDAS_LOG_ERROR("failed to set keep_alive socket option " << e3 << *this);
            }

            int32_t linger = Config::instance().get<int32_t>(cfgPath() + ".linger", 0);
            boost::system::error_code e4;
            skt.set_option(boost::asio::socket_base::linger((0 < linger), linger), e4);
            if (e4) {
                MIDAS_LOG_ERROR("failed to set linger socket option " << e4 << *this);
            }

            int readBufSize = Config::instance().get<int>(cfgPath() + ".rcvbuf", 2097152);
            if (0 < readBufSize) {
                boost::system::error_code e5;
                skt.set_option(boost::asio::socket_base::receive_buffer_size(readBufSize), e5);
                if (e5) {
                    MIDAS_LOG_ERROR("failed to set receive_buffer_size socket option " << e5 << *this);
                }
            }

            int writeBufSize = Config::instance().get<int>(cfgPath() + ".sndbuf", 262144);
            if (0 < writeBufSize) {
                boost::system::error_code e6;
                skt.set_option(boost::asio::socket_base::send_buffer_size(writeBufSize), e6);
                if (e6) {
                    MIDAS_LOG_ERROR("failed to set send_buffer_size socket option " << e6 << *this);
                }
            }

            bool isNoDelay = Config::instance().get<bool>(cfgPath() + ".nodelay", true);
            boost::system::error_code ec7;
            skt.set_option(boost::asio::ip::tcp::no_delay(isNoDelay), ec7);
            if (ec7) {
                MIDAS_LOG_ERROR("error turn on no delay option for " << isNoDelay << " " << *this);
            }

            bool nonBlocking = Config::instance().get<bool>(cfgPath() + ".nonBlocking", true);
            boost::system::error_code ec8;
            skt.non_blocking(nonBlocking, ec8);
            if (ec8) {
                MIDAS_LOG_ERROR("failed to set nonBlocking socket option " << nonBlocking << " " << *this);
            }

            bool nativeNonBlocking = Config::instance().get<bool>(cfgPath() + ".nativeNonBlocking", true);
            boost::system::error_code ec9;
            skt.native_non_blocking(nativeNonBlocking, ec9);
            if (ec9) {
                MIDAS_LOG_ERROR("failed to set nativeNonBlocking socket option " << nativeNonBlocking << " " << *this);
            }
        }

        void reset() {
            boost::system::error_code e;
            skt.close(e);
            Member::reset_recv_stats();
            Member::reset_sent_stats();
            Member::set_state(NetState::Closed);
            MIDAS_LOG_INFO("this tcp recv reset " << *this);
        }

        void set_name(bool reset) {
            if (reset) {
                for (size_t i = 0; i < Member::MaxNameNumber; ++i) {
                    Member::set_name(string(), i);
                }
                Member::use_name(0);
                tcpAddrIndex_ = 0;
            }

            if (isConnectable) {
                for (size_t i = 0; i < addrVec.size(); ++i) {
                    Member::set_name(addrVec[i].host + ":" + addrVec[i].service, i);
                }
            } else {
                Member::set_name(remoteEndpoint.address().to_string() + ":" + int2str(remoteEndpoint.port()), 0);
            }
        }

        void use_name(size_t index) {
            Member::use_name(index);
            tcpAddrIndex_ = index;
        }

        string cfgPath() const { return (isConnectable ? addrVec[tcpAddrIndex_].cfg : cfg); }

        virtual void stats(std::ostream& os) {
            os << std::left << std::setw(25) << "tcp connection" << std::setw(25) << "description" << std::setw(25)
               << "connect/accept" << std::setw(25) << "local endpoint" << std::setw(25) << "remote endpoint"
               << std::setw(25) << "connected time" << std::right << std::setw(25) << "msg sent" << std::setw(25)
               << "bytes sent" << std::setw(25) << "msg recv" << std::setw(25) << "bytes recv" << '\n';
            my_stats(os);
        }

        void my_stats(std::ostream& os) const {
            if (isConnectable || skt.is_open()) {
                ostringstream oss;
                oss << localEndpoint;
                string localEndpointStr = oss.str();
                oss.clear();
                oss << remoteEndpoint;
                string remoteEndpointStr = oss.str();

                string connectTimeStr = time_t2string(Member::connectTime);

                os << std::left << std::setw(25) << Member::get_name().substr(0, 24) << std::setw(25)
                   << Member::get_description().substr(0, 24) << std::setw(25) << get_bool_string(isConnectable)
                   << std::setw(25) << localEndpointStr.substr(0, 24) << std::setw(25)
                   << remoteEndpointStr.substr(0, 24) << std::setw(25) << connectTimeStr.substr(0, 24) << std::right
                   << std::setw(25) << Member::msgsSent << std::setw(25) << Member::bytesSent << std::setw(25)
                   << Member::msgsRecv << std::setw(25) << Member::bytesRecv << '\n';
            }
        }

        void list(std::ostream& os) const {
            if (isConnectable || skt.is_open()) {
                string name = group_.get_name() + "/" + Member::get_name();
                ostringstream oss;
                oss << localEndpoint;
                string localEndpointStr = oss.str();
                oss.clear();
                oss << remoteEndpoint;
                string remoteEndpointStr = oss.str();

                string connectTimeStr = time_t2string(Member::connectTime);

                os << std::left << std::setw(25) << Member::get_name().substr(0, 24) << std::setw(25)
                   << Member::get_description().substr(0, 24) << std::setw(25) << get_bool_string(isConnectable)
                   << std::setw(25) << localEndpointStr.substr(0, 24) << std::setw(25)
                   << remoteEndpointStr.substr(0, 24) << std::setw(25) << connectTimeStr.substr(0, 24) << '\n';
            }
        }

        template <typename Allocator>
        static TcpRecv::SharedPtr create(TcpRecvGroup& group, const vector<TcpAddress>& addrList, const string& cfg,
                                         boost::asio::io_service& iosvc, boost::asio::io_service::strand* strand,
                                         size_t bs, const Allocator allocator) {
            return TcpRecv::SharedPtr(new TcpRecv(group, addrList, cfg, iosvc, strand, bs, allocator));
        }

        static bool default_connect_callback() { return true; }
        static bool default_disconnect_callback() { return true; }
        static bool default_data_callback() { return true; }
        static bool default_retry_callback() { return true; }

        static TConnectCallback& connect_callback() {
            static TConnectCallback callback = boost::bind(&TcpRecv::default_connect_callback);
            return callback;
        }
        static TDisconnectCallback& disconnect_callback() {
            static TDisconnectCallback callback = boost::bind(&TcpRecv::default_disconnect_callback);
            return callback;
        }
        static TDataCallback& data_callback() {
            static TDataCallback callback = boost::bind(&TcpRecv::default_data_callback);
            return callback;
        }
        static TRetryCallback& retry_callback() {
            static TRetryCallback callback = boost::bind(&TcpRecv::default_retry_callback);
            return callback;
        }
    };
    // end

    struct TcpRecvStart {
        void operator()(TcpRecv::SharedPtr tcpRecv) const {
            tcpRecv->strand_->post(boost::bind(&TcpRecv::async_start, tcpRecv));
        }
    };
    struct TcpRecvStop {
        void operator()(TcpRecv::SharedPtr tcpRecv) const {
            tcpRecv->strand_->post(boost::bind(&TcpRecv::async_stop, tcpRecv));
        }
    };
    struct TcpRecvIsOpen {
        bool operator()(TcpRecv::SharedPtr tcpRecv) const { return tcpRecv->skt.is_open(); }
    };
    struct TcpRecvIsClose {
        bool operator()(TcpRecv::SharedPtr tcpRecv) const { return !(tcpRecv->skt.is_open()); }
    };

    struct TcpRecvGroupList {
        ostream& os;
        explicit TcpRecvGroupList(ostream& o) : os(o) {}
        void operator()(Member::SharedPtr member) const {
            TcpRecvGroup::SharedPtr ptr = dynamic_pointer_cast<TcpRecvGroup>(member);
            if (ptr) ptr->list(os);
        }
    };

    // end struct

public:
    typedef boost::intrusive_ptr<TcpRecvGroup> SharedPtr;
    typedef boost::function<bool(TcpRecv&)> TAcceptCallback;
    typedef tbb::concurrent_vector<TcpRecv::SharedPtr> TTcpRecvVec;
    typedef boost::function<TcpRecv::SharedPtr(TcpRecvGroup&, const vector<TcpAddress>&, const string&,
                                               boost::asio::io_service&, boost::asio::io_service::strand*, size_t)>
        TCreateTcpRecv;

    const string cfg;
    bool running{false};
    CChannel& channel;
    const ssize_t slot_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::endpoint endpoint_;
    boost::scoped_ptr<boost::asio::io_service::strand> strandPtr;
    boost::asio::io_service::strand* strand_;
    TAcceptCallback acceptCallback;
    TTcpRecvVec tcpRecvVec1;
    TTcpRecvVec tcpRecvVec2;
    TCreateTcpRecv createTcpRecv;
    const size_t bufferSize;
    static SharedPtr tcpRecvGroup_ __attribute__((weak));

public:
    template <typename Allocator>
    TcpRecvGroup(const string& cfg_, CChannel& c, boost::asio::io_service::strand* strand = nullptr,
                 size_t bufferSize_ = 1048576, const Allocator& allocator = Allocator())
        : cfg(cfg_),
          channel(c),
          slot_(c.get_slot()),
          acceptor_(c.iosvc),
          strandPtr(strand ? nullptr : new boost::asio::io_service::strand(c.iosvc)),
          strand_(strand ? strand : strandPtr.get()),
          acceptCallback(accept_callback()),
          createTcpRecv(boost::bind(&TcpRecv::create<Allocator>, _1, _2, _3, _4, _5, _6, allocator)),
          bufferSize(bufferSize_) {
        netProtocol = NetProtocol::p_tcp_primary;
        displayStat = false;
    }

    virtual ~TcpRecvGroup() { channel.clear_slot(slot_); }

    static TcpRecvGroup::SharedPtr new_instance(const string& cfg_, CChannel& c,
                                                boost::asio::io_service::strand* strand = nullptr,
                                                size_t bufferSize = 1048576) {
        return new_instance(cfg_, c, strand, bufferSize, HeapAllocator());
    }

    template <typename Allocator>
    static TcpRecvGroup::SharedPtr new_instance(const string& cfg_, CChannel& c,
                                                boost::asio::io_service::strand* strand = nullptr,
                                                size_t bufferSize = 1048576, const Allocator allocator = Allocator()) {
        TcpRecvGroup::SharedPtr tmp(new TcpRecvGroup(cfg_, c, strand, bufferSize, allocator));
        return tmp;
    }

    static string& default_config_path() {
        static string path{"midas.io.tcp_in"};
        return path;
    }
    static void default_config_path(const string& path) { default_config_path() = path; }

    void start(const string& service) {
        boost::asio::ip::tcp::resolver resolver(channel.iosvc);
        boost::asio::ip::tcp::resolver::query query("127.0.0.1", service);
        boost::asio::ip::tcp::endpoint ep = *resolver.resolve(query);
        strand_->post(boost::bind(&TcpRecvGroup::async_start, TcpRecvGroup::SharedPtr(this), ep, service));
    }

    void start(unsigned short port) {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);
        strand_->post(boost::bind(&TcpRecvGroup::async_start, TcpRecvGroup::SharedPtr(this), ep, int2str(port)));
    }

    void start() {
        strand_->post(boost::bind(&TcpRecvGroup::async_start, TcpRecvGroup::SharedPtr(this),
                                  boost::asio::ip::tcp::endpoint(), string()));
    }

    void stop() { strand_->post(boost::bind(&TcpRecvGroup::async_stop, TcpRecvGroup::SharedPtr(this))); }

    /**
     * create a TcpRecv instance which is connectable. a raw pointer is returned because TcpRecvGroup
     * has sole ownership. TcpRecv instance will auto connect if connect parameter is true.
     */
    TcpRecv* new_tcp_recv(const vector<TcpAddress>& vec, const TcpRecv::TConnectCallback ccb,
                          const TcpRecv::TRetryCallback rcb, bool connect = true) {
        if (vec.empty() || vec[0].host.empty() || vec[0].service.empty()) return nullptr;

        TcpRecv::SharedPtr tcpRecv = createTcpRecv(*this, vec, string(), channel.iosvc, nullptr, bufferSize);
        tcpRecv->connect_callback(ccb, CustomCallbackTag());
        tcpRecv->retry_callback(rcb, CustomCallbackTag());
        strand_->post(boost::bind(&TcpRecvGroup::async_join, TcpRecvGroup::SharedPtr(this), tcpRecv, connect));
        return tcpRecv.get();
    }

    TcpRecv* new_tcp_recv(const vector<TcpAddress>& vec, bool connect = true) {
        if (vec.empty() || vec[0].host.empty() || vec[0].service.empty()) return nullptr;

        TcpRecv::SharedPtr tcpRecv = createTcpRecv(*this, vec, string(), channel.iosvc, nullptr, bufferSize);
        strand_->post(boost::bind(&TcpRecvGroup::async_join, TcpRecvGroup::SharedPtr(this), tcpRecv, connect));
        return tcpRecv.get();
    }

    void deliver(const ConstBuffer& msg, ssize_t slot) {
        if (slot_ == slot) {
            std::for_each(tcpRecvVec1.begin(), tcpRecvVec1.end(), boost::bind(&TcpRecv::deliver, _1, boost::cref(msg)));
            std::for_each(tcpRecvVec2.begin(), tcpRecvVec2.end(), boost::bind(&TcpRecv::deliver, _1, boost::cref(msg)));
        }
    }

    void accept_callback(const TAcceptCallback acb, const CustomCallbackTag&) { acceptCallback = acb; }

    static void accept_callback(const TAcceptCallback acb) { accept_callback() = acb; }

    template <typename Stream>
    void print(Stream& s) const {
        s << " tcpRecvGroup[ name[" << Member::get_name() << "] desc[" << Member::get_description() << "] enabled["
          << get_bool_string(enabled) << "] running[" << get_bool_string(running) << "] endpoint[" << endpoint_ << "]]";
    }

    static void register_admin(CChannel& channel, AdminHandler& handler, const string& groupName = string(),
                               const string& cfgPath = string()) {
        string listServerCmd{"list_server"};
        string openServerCmd{"open_server"};
        string killServerCmd{"kill_server"};

        if (!groupName.empty()) {
            listServerCmd += "_" + groupName;
            openServerCmd += "_" + groupName;
            killServerCmd += "_" + groupName;
        }

        handler.register_callback(
            listServerCmd, boost::bind(&TcpRecvGroup::admin_list, _1, _2, boost::ref(channel), groupName, cfgPath),
            "list all tcp connection", "usage: " + listServerCmd);
        handler.register_callback(
            openServerCmd, boost::bind(&TcpRecvGroup::admin_connect, _1, _2, boost::ref(channel), groupName, cfgPath),
            "open a tcp connection", "usage: " + openServerCmd + " host:svc[:interface] [cfg]");
        handler.register_callback(killServerCmd, boost::bind(&TcpRecvGroup::admin_disconnect, _1, _2,
                                                             boost::ref(channel), groupName, cfgPath),
                                  "close all tcp connection", "usage: " + killServerCmd + " host:svc");

        tcpRecvGroup_ = new_instance(string(), channel);
        assert(tcpRecvGroup_);
        tcpRecvGroup_->start();
    }

    void async_start(boost::asio::ip::tcp::endpoint ep, string name) {
        if (running) return;
        Member::set_name(":" + name);
        Member::set_state(NetState::Pending);
        running = true;
        channel.join_slot<TcpRecvGroup>(TcpRecvGroup::SharedPtr(this), slot_);

        if (boost::asio::ip::tcp::endpoint() != ep) {
            boost::system::error_code e1;
            acceptor_.open(ep.protocol(), e1);
            if (e1) {
                MIDAS_LOG_ERROR("failed to open acceptor " << e1 << *this);
                return;
            }

            if (0 < ep.port()) {
                boost::system::error_code e2;
                acceptor_.set_option(boost::asio::socket_base::reuse_address(true), e2);
                if (e2) {
                    MIDAS_LOG_ERROR("failed to set option for acceptor " << e2 << *this);
                    return;
                }

                boost::system::error_code e3;
                acceptor_.bind(ep, e3);
                if (e3) {
                    MIDAS_LOG_ERROR("failed to bind acceptor " << e3 << *this);
                    return;
                }
            }

            boost::system::error_code e4;
            acceptor_.listen(boost::asio::socket_base::max_connections, e4);
            if (e4) {
                MIDAS_LOG_ERROR("failed to listen acceptor " << e4 << *this);
                return;
            }

            boost::system::error_code e5;
            endpoint_ = acceptor_.local_endpoint(e5);
            if (e5) {
                MIDAS_LOG_ERROR("failed to retrieve local_endpoint " << e5 << *this);
            }

            MIDAS_LOG_INFO("start to accept at " << ep);
            accept();
        }

        std::for_each(tcpRecvVec1.begin(), tcpRecvVec1.end(), TcpRecvStart());
    }

    void async_stop() {
        if (!running) return;

        if (boost::asio::ip::tcp::endpoint() != endpoint_) {
            boost::system::error_code e;
            acceptor_.close(e);
            if (e) {
                MIDAS_LOG_ERROR("failed to close acceptor " << e << *this);
            }
            endpoint_ = boost::asio::ip::tcp::endpoint();
        }
        channel.leave_slot<TcpRecvGroup>(TcpRecvGroup::SharedPtr(this), slot_);
        running = false;
        Member::set_name(string());
        std::for_each(tcpRecvVec2.begin(), tcpRecvVec2.end(), TcpRecvStop());
        std::for_each(tcpRecvVec1.begin(), tcpRecvVec1.end(), TcpRecvStop());
    }

    void async_join(TcpRecv::SharedPtr tcpRecv, bool connect) {
        tcpRecvVec1.push_back(tcpRecv);
        if (connect) {
            tcpRecv->connect();
        }
    }

    void accept() {
        if (!running) return;
        TcpRecv::SharedPtr tcpRecv;
        auto it = std::find_if(tcpRecvVec2.begin(), tcpRecvVec2.end(), TcpRecvIsClose());
        if (tcpRecvVec2.end() == it) {
            tcpRecv = createTcpRecv(*this, vector<TcpAddress>(), cfg, channel.iosvc, nullptr, bufferSize);
            tcpRecvVec2.push_back(tcpRecv);
        } else {
            tcpRecv = *it;
        }

        tcpRecv->localEndpoint = boost::asio::ip::tcp::endpoint();
        tcpRecv->remoteEndpoint = boost::asio::ip::tcp::endpoint();
        acceptor_.async_accept(tcpRecv->skt, strand_->wrap(boost::bind(&TcpRecvGroup::on_accept,
                                                                       TcpRecvGroup::SharedPtr(this), tcpRecv, _1)));
    }

    void on_accept(TcpRecv::SharedPtr tcpRecv, const boost::system::error_code& e) {
        if (!running) return;

        if (e) {
            MIDAS_LOG_ERROR("failed to accept " << e << tcpRecv << *this);
            if (boost::asio::error::operation_aborted == e) return;
            tcpRecv->reset();
            tcpRecv->localEndpoint = boost::asio::ip::tcp::endpoint();
            tcpRecv->remoteEndpoint = boost::asio::ip::tcp::endpoint();
            tcpRecv->mutableBuffer.clear();
            tcpRecv->bufferWriter.reset();

            accept();
            return;
        }

        MIDAS_LOG_INFO("the tcp recv is started " << *tcpRecv);
        boost::system::error_code e1;
        tcpRecv->localEndpoint = tcpRecv->skt.local_endpoint(e1);
        if (e1) {
            MIDAS_LOG_ERROR("failed to retrieve its local endpoint " << e1 << tcpRecv << *this);
        }
        boost::system::error_code e2;
        tcpRecv->remoteEndpoint = tcpRecv->skt.remote_endpoint(e2);
        if (e2) {
            MIDAS_LOG_ERROR("failed to retrieve its remote endpoint " << e2 << tcpRecv << *this);
        }

        tcpRecv->set_name(true);

        if (!enabled || !acceptCallback(*tcpRecv)) {
            MIDAS_LOG_INFO("failed to accept a tcpRecv because its accept callback reject with false");
            tcpRecv->reset();
            tcpRecv->localEndpoint = boost::asio::ip::tcp::endpoint();
            tcpRecv->remoteEndpoint = boost::asio::ip::tcp::endpoint();
            tcpRecv->mutableBuffer.clear();
            tcpRecv->bufferWriter.reset();
            accept();
            return;
        }
        tcpRecv->set_socket_option();
        tcpRecv->strand_->post(boost::bind(&TcpRecv::async_start, tcpRecv));
    }

    template <typename UnaryPredicate>
    TcpRecv::SharedPtr find_tcp_recv(const UnaryPredicate& unaryPredicate) {
        TcpRecv::SharedPtr tcpRecv = find_tcp_recv1(unaryPredicate);
        if (tcpRecv) return tcpRecv;
        return find_tcp_recv2(unaryPredicate);
    }

    template <typename UnaryPredicate>
    TcpRecv::SharedPtr find_tcp_recv1(const UnaryPredicate& unaryPredicate) {
        auto it = std::find_if(tcpRecvVec1.begin(), tcpRecvVec1.end(), unaryPredicate);
        if (tcpRecvVec1.end() != it) return *it;
        return nullptr;
    }

    template <typename UnaryPredicate>
    TcpRecv::SharedPtr find_tcp_recv2(const UnaryPredicate& unaryPredicate) {
        auto it = std::find_if(tcpRecvVec2.begin(), tcpRecvVec2.end(), unaryPredicate);
        if (tcpRecvVec2.end() != it) return *it;
        return TcpRecv::SharedPtr();
    }

    virtual void stats(std::ostream& os) {
        os << std::left << std::setw(25) << "tcp group" << std::setw(25) << "description" << std::setw(25) << "enabled"
           << std::setw(25) << "endpoint" << std::setw(25) << "status" << std::right << std::setw(25) << "msg sent"
           << std::setw(25) << "bytes sent" << std::setw(25) << "msg recv" << std::setw(25) << "bytes recv" << '\n';

        os << std::left << std::setw(25) << Member::get_name().substr(0, 24) << std::setw(25)
           << Member::get_description().substr(0, 24) << std::setw(25) << get_bool_string(enabled) << std::setw(25)
           << object2str(endpoint_).substr(0, 24) << std::setw(25) << (running ? "started" : "stopped") << std::right
           << std::setw(25) << Member::msgsSent << std::setw(25) << Member::bytesSent << std::setw(25)
           << Member::msgsRecv << std::setw(25) << Member::bytesRecv << '\n';

        os << std::left << std::setw(25) << "tcp connection" << std::setw(25) << "description" << std::setw(25)
           << "connect/accept" << std::setw(25) << "local endpoint" << std::setw(25) << "remote endpoint"
           << std::setw(25) << "connected time" << std::right << std::setw(25) << "msg sent" << std::setw(25)
           << "bytes sent" << std::setw(25) << "msg recv" << std::setw(25) << "bytes recv" << '\n';

        std::for_each(tcpRecvVec1.begin(), tcpRecvVec1.end(), boost::bind(&TcpRecv::my_stats, _1, boost::ref(os)));
        os << '\n';
    }

    void list(std::ostream& os) const {
        os << std::left << std::setw(25) << "tcp group" << std::setw(25) << "description" << std::setw(25) << "enabled"
           << std::setw(25) << "endpoint" << std::setw(25) << "status" << std::setw(25) << "config" << std::right
           << std::setw(25) << "max retry count" << std::setw(25) << "max retry time" << '\n';

        size_t maxRetryCount = Config::instance().get<size_t>(cfg + ".max_retry", std::numeric_limits<size_t>::max());
        boost::posix_time::time_duration maxRetryTime = Config::instance().get<boost::posix_time::time_duration>(
            cfg + ".max_retry_time", boost::posix_time::seconds(30));

        os << std::left << std::setw(25) << Member::get_name().substr(0, 24) << std::setw(25)
           << Member::get_description().substr(0, 24) << std::setw(25) << get_bool_string(enabled) << std::setw(25)
           << object2str(endpoint_).substr(0, 24) << std::setw(25) << (running ? "started" : "stopped") << std::setw(25)
           << cfg.substr(0, 24) << std::right << std::setw(25) << maxRetryCount << std::setw(25)
           << object2str(maxRetryTime).substr(0, 24) << '\n'
           << '\n';

        os << std::left << std::setw(25) << "tcp connection" << std::setw(25) << "description" << std::setw(25)
           << "connect/accept" << std::setw(25) << "local endpoint" << std::setw(25) << "remote endpoint"
           << std::setw(25) << "connected time" << '\n';
        std::for_each(tcpRecvVec1.begin(), tcpRecvVec1.end(), boost::bind(&TcpRecv::list, _1, boost::ref(os)));
        std::for_each(tcpRecvVec2.begin(), tcpRecvVec2.end(), boost::bind(&TcpRecv::list, _1, boost::ref(os)));
        os << '\n';
    }

    static bool default_accept_callback() { return true; }

    static TAcceptCallback& accept_callback() {
        static TAcceptCallback cb = boost::bind(&TcpRecvGroup::default_accept_callback);
        return cb;
    }

    static string admin_list(const string& cmd, const TAdminCallbackArgs& args, CChannel& channel, const string& group,
                             const string& cfg) {
        ostringstream os;
        channel.for_all_members(TcpRecvGroupList(os));
        return os.str();
    }

    static string admin_connect(const string& cmd, const TAdminCallbackArgs& args, CChannel& channel,
                                const string& group, const string& cfg) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string hostService1{args[0]};
        string hostService2;
        string cfg1(cfg.empty() ? TcpRecvGroup::default_config_path() : cfg);
        string cfg2(cfg.empty() ? TcpRecvGroup::default_config_path() : cfg);
        string desc_{group};
        string s1(1 < args.size() ? args[1] : string());
        string s2(2 < args.size() ? args[2] : string());
        string s3(3 < args.size() ? args[3] : string());
        string s4(4 < args.size() ? args[4] : string());

        if (!s1.empty()) {
            if (string::npos != s1.find(':')) {
                hostService2 = s1;
                if ("null" != Config::instance().get<string>(s2, "null")) {
                    cfg2 = s2;
                } else {
                    if (!group.empty()) {
                        desc_ += "/";
                    }
                    desc_ += s2;
                }
            } else if ("null" != Config::instance().get<string>(s1, "null")) {
                cfg1 = s1;
                if (string::npos != s2.find(':')) {
                    hostService2 = s2;
                    if ("null" != Config::instance().get<string>(s3, "null")) {
                        cfg2 = s3;
                    } else {
                        if (!group.empty()) {
                            desc_ += "/";
                        }
                        desc_ += s3;
                    }
                } else {
                    if (!group.empty()) {
                        desc_ += "/";
                    }
                    desc_ += s2;
                }
            } else {
                if (!group.empty()) {
                    desc_ += "/";
                }
                desc_ += s1;
            }
        }

        vector<TcpAddress> v;
        string h1, svc1, itf1;
        split_host_port_interface(hostService1, h1, svc1, itf1);

        string h2, svc2, itf2;
        split_host_port_interface(hostService2, h2, svc2, itf2);

        MIDAS_LOG_INFO("parameters " << h1 << ":" << svc1 << ":" << itf1 << " " << h2 << ":" << svc2 << ":" << itf2
                                     << " " << cfg1 << " " << cfg2);

        if (h1.empty() || svc1.empty()) {
            return "failed to open connection to " + args[0];
        }

        v.push_back(TcpAddress(h1, svc1, itf1, cfg1));
        if (!h2.empty() && !svc2.empty()) {
            v.push_back(TcpAddress(h2, svc2, itf2, cfg2));
        }

        if (channel.get_member(h1 + ":" + svc1)) {
            return "connection already exist for " + h1 + ":" + svc1;
        }

        TcpRecv* tcpRecv = tcpRecvGroup_->new_tcp_recv(v, false);
        assert(tcpRecv);
        tcpRecv->set_description(desc_);
        tcpRecv->connect();
        return "new connection for " + tcpRecv->get_name() + " (" + desc_ + " cfg: " + cfg1 + ")";
    }

    static string admin_disconnect(const string& cmd, const TAdminCallbackArgs& args, CChannel& channel,
                                   const string& group, const string& cfg) {
        if (args.empty()) return MIDAS_ADMIN_HELP_RESPONSE;
        string name{args[0]};
        Member::SharedPtr member = channel.get_member(name);
        TcpRecv::SharedPtr tcpRecv = dynamic_pointer_cast<TcpRecv>(member);
        if (!tcpRecv) return "connection not found " + name;
        tcpRecv->disconnect();
        return "connection closed for " + name;
    }
};

TcpRecvGroup::SharedPtr TcpRecvGroup::tcpRecvGroup_;

template <typename Stream>
inline Stream& operator<<(Stream& s, const TcpRecvGroup::TcpRecv& tcpRecv) {
    tcpRecv.print(s);
    return s;
}

template <typename Stream>
inline Stream& operator<<(Stream& s, const boost::system::error_code& e) {
    s << " error code[ category[" << e.category().name() << "] msg[" << e.message() << "]]";
    return s;
}

template <typename Stream>
inline Stream& operator<<(Stream& s, const TcpRecvGroup& tcpRecvGroup) {
    tcpRecvGroup.print(s);
    return s;
}
}

#endif
