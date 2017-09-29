#ifndef MIDAS_PUBLISHER_ASYNC_H
#define MIDAS_PUBLISHER_ASYNC_H

#include <tbb/atomic.h>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <sstream>
#include "net/buffer/BufferManager.h"
#include "net/buffer/ConstBuffer.h"
#include "net/buffer/ConstBufferSequence.h"
#include "net/channel/Channel.h"
#include "utils/Backoff.h"

using namespace std;

namespace midas {

/**
 * this will do async write and msg caching. the deliver call will place msg in buffer sequence and
 * io thread will consume msgs to send to clients.
 * if total pending msgs exceed given threshold, the connection will be closed
 * slow clients will be disconnected
 */
template <typename Protocol, typename Derived, template <typename, typename> class PublisherBase,
          bool ThreadPerConnection, size_t WriteThreshold>
class PublisherAsync : public PublisherBase<Protocol, Derived> {
public:
    typedef PublisherBase<Protocol, Derived> TPublisherBase;
    bool isWorkerStarted{false};
    long writeThreshold;         // max number of bytes which can be cached
    uint64_t pollTime;           // poll time for non-blocking mode before block
    tbb::atomic<bool> sendFlag;  // indicate in progress sending
    BufferManager buffer;

public:
    PublisherAsync(CChannel& channel, const string& cfg)
        : TPublisherBase(channel, cfg),
          writeThreshold(Config::instance().get<long>(cfg + ".writelist_threshold", WriteThreshold)),
          pollTime(Config::instance()
                       .get<boost::posix_time::time_duration>(cfg + ".poll_time", boost::posix_time::microseconds(100))
                       .total_nanoseconds()) {
        sendFlag = false;
    }

    ~PublisherAsync() {}

    void stats(ostream& os) {
        TPublisherBase::stats(os);

        os << "threshold        = " << writeThreshold << endl
           << "poll time        = " << pollTime / 1000 << " usec" << endl
           << "bytes wait       = " << buffer.pending_bytes() << endl;
    }

    /**
     * async send given msg to connected client, place msg in BufferManager then return
     */
    void deliver(const ConstBuffer& msg) {
        if (this->closed()) return;

        if (sendFlag.compare_and_swap(true, false)) {
            long pendingBytes = buffer.push_back(msg);  // multiply producer can push
            if (pendingBytes > writeThreshold) {
                MIDAS_LOG_WARNING("slow client detected " << this->get_name() << " closing connection...");
                this->close();
                return;
            }
            if (!sendFlag.compare_and_swap(true, false)) {
                // no other consumer can run at this stage so safe to swap
                this->_strand.post(
                    boost::bind(&PublisherAsync::async_send<ConstBufferSequence>, (Derived*)this, buffer.swap()));
            }
        } else {
            // no other consumer can run at this stage so safe to swap
            if (!buffer.empty()) {
                buffer.push_back(msg);
                this->_strand.post(
                    boost::bind(&PublisherAsync::async_send<ConstBufferSequence>, (Derived*)this, buffer.swap()));
            } else {
                this->_strand.post(boost::bind(&PublisherAsync::async_send<ConstBuffer>, (Derived*)this, msg));
            }
        }
    }

    long pending_bytes() const { return buffer.pending_bytes(); }

    long pending_buffers() const { return buffer.pending_buffers(); }

    void init() {
        TPublisherBase::init();
        if (ThreadPerConnection) isWorkerStarted = this->channel.work();
    }

    void stop() {
        TPublisherBase::stop();
        if (isWorkerStarted)
            this->_strand.post(
                boost::bind(&PublisherAsync::stop_worker, typename TPublisherBase::SharedPtr((Derived*)this)));
    }

    void stop_worker() {
        if (isWorkerStarted) {
            isWorkerStarted = false;
            throw StopRunTag();
        }
    }

    template <typename BufferType>
    void async_send(const BufferType& b) {
        try {
            boost::asio::async_write(
                this->skt, b, this->_strand.wrap(boost::bind(
                                  &PublisherAsync::on_write, typename TPublisherBase::SharedPtr((Derived*)this),
                                  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        } catch (const boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error write to socket " << this->get_name() << " : " << e.what());
            this->close();
        }
    }

    void on_write(const boost::system::error_code& ec, size_t s) {
        if (this->closed()) return;
        this->add_sent_msg_2_stat(s);
        if (s > this->peekBytesSent) this->peekBytesSent = s;

        if (!ec) {
            // try spin wait to get next msg
            if (buffer.empty() && pollTime) {
                uint64_t pollStartTime = ntime();
                BackOff backOff;
                while (buffer.empty() && (ntime() - pollStartTime) < pollTime) backOff.pause();
            }

            if (!buffer.empty())
                async_send(buffer.swap());
            else {
                sendFlag = false;
                // make sure that other thread do not put something on queue
                if (!buffer.empty() && !sendFlag.compare_and_swap(true, false)) {
                    async_send(buffer.swap());
                }
            }
        } else {
            sendFlag = false;
            MIDAS_LOG_ERROR("error write to socket " << this->get_name() << " : " << ec);
            this->close();
        }
    }

    friend class PublisherBase<Protocol, Derived>;
};
}

#endif
