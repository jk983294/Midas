#ifndef MIDAS_UDP_PUBLISHER_DIRECT_H
#define MIDAS_UDP_PUBLISHER_DIRECT_H

#include "net/udp/UdpPublisherBase.h"

namespace midas {

/**
 * this will send msg directly. the deliver call will call send function from caller thread
 * this will result in loewe latency but average throughput might be lower than async send
 * recommended for low msg rate
 *
 * currently only support multicast
 */
template <typename Tag = DefaultUdpPublisherTag>
class UdpPublisherDirect : public UdpPublisherBase<UdpPublisherDirect<Tag>> {
public:
    typedef UdpPublisherBase<UdpPublisherDirect> PublisherBase;
    typedef boost::intrusive_ptr<UdpPublisherDirect> SharedPtr;
    template <typename D>
    friend class UdpPublisherBase;

    LockType::mutex mtx;

public:
    ~UdpPublisherDirect() {}

    void deliver(const ConstBuffer& msg) {
        LockType::scoped_lock guard(mtx);

        if (this->is_closed() || !this->is_multicast()) return;
        if (msg.size() > this->peekBytesSent) {
            this->peekBytesSent = msg.size();
        }

        try {
            if (this->skt.send(msg) != msg.size()) {
                MIDAS_LOG_ERROR("error writing to socket" << this->get_name());
                this->stop();
            } else {
                this->add_sent_msg_2_stat(msg.size());
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error writing to socket" << this->get_name() << " : " << e.what());
            this->stop();
        }
    }

    static string& default_config_path() {
        static string _path{"midas.io.udp_out"};
        return _path;
    }

private:
    UdpPublisherDirect(const string& ip, const string& port, const string& itf, CChannel& output_channel,
                       const string& cfg)
        : PublisherBase(ip, port, itf, output_channel, cfg) {}
};
}

#endif
