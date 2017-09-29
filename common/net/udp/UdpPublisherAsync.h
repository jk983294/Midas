#ifndef MIDAS_UDP_PUBLISHER_ASYNC_H
#define MIDAS_UDP_PUBLISHER_ASYNC_H

#include <tbb/atomic.h>
#include <boost/asio/placeholders.hpp>
#include <vector>
#include "net/buffer/BufferManager.h"
#include "net/common/TimerMember.h"
#include "net/udp/UdpPublisherBase.h"
#include "utils/Backoff.h"

namespace midas {

template <typename Tag = DefaultUdpPublisherTag>
class UdpPublisherAsync : public UdpPublisherBase<UdpPublisherAsync<Tag>> {
public:
    typedef UdpPublisherBase<UdpPublisherAsync> PublisherBase;
    typedef boost::intrusive_ptr<UdpPublisherAsync> SharedPtr;
    template <typename D>
    friend class UdpPublisherBase;

    tbb::atomic<bool> sendingFlag;
    uint64_t pollingTime;
    BufferManager outputBuffer;

public:
    ~UdpPublisherAsync() {}

    void stop() {
        // wait while async send finish
        while (!sendingFlag.compare_and_swap(true, false))
            ;
        PublisherBase::stop();
    }

    void deliver(const ConstBuffer& msg) {
        if (this->is_closed() || !this->is_multicast()) return;

        if (sendingFlag.compare_and_swap(true, false)) {
            // multiple producer can push
            outputBuffer.push_back(msg);

            if (!sendingFlag.compare_and_swap(true, false)) {
                // it is safe to swap since no other consumer thread can run here
                async_send(outputBuffer.swap());
            }
        } else {
            if (!outputBuffer.empty()) {
                outputBuffer.push_back(msg);
                async_send(outputBuffer.swap());
            } else {
                async_send(msg);
            }
        }
    }

    static string& default_config_path() {
        static string _path{"midas.io.udp_out"};
        return _path;
    }

    void stats(ostream& os) {
        PublisherBase::stats(os);

        os << "polling time " << (pollingTime / 1000) << " usec" << endl
           << "bytes wait " << outputBuffer.pending_bytes() << endl;
    }

private:
    UdpPublisherAsync(const string& ip, const string& port, const string& itf, CChannel& output_channel,
                      const string& cfg)
        : PublisherBase(ip, port, itf, output_channel, cfg),
          pollingTime(
              Config::instance()
                  .get<boost::posix_time::time_duration>(cfg + ".polling_time", boost::posix_time::microseconds(100))
                  .total_nanoseconds()) {}

    template <typename TBuffer>
    void async_send(const TBuffer& buffer) {
        try {
            this->skt.async_send(
                buffer, boost::bind(&UdpPublisherAsync::on_write, SharedPtr(this), boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error writing to socket " << this->get_name() << " : " << e.what());
            PublisherBase::stop();
        }
    }

    void on_write(const boost::system::error_code& e, size_t s) {
        this->add_sent_msg_2_stat(s);
        if (s > this->peekBytesSent) {
            this->peekBytesSent = s;
        }

        if (!e) {
            if (outputBuffer.empty() && pollingTime) {
                uint64_t pollStartTime = ntime();
                BackOff backOff;
                while (outputBuffer.empty() && (ntime() - pollStartTime) < pollingTime) {
                    backOff.pause();
                }
            }

            if (!outputBuffer.empty()) {
                async_send(outputBuffer.swap());
            } else {
                sendingFlag = false;
                if (!outputBuffer.empty() && !sendingFlag.compare_and_swap(true, false)) {
                    async_send(outputBuffer.swap());
                }
            }
        } else {
            sendingFlag = false;
            MIDAS_LOG_ERROR("error writing to socket " << this->get_name() << " : " << e);
            PublisherBase::stop();
        }
    }
};
}

#endif
