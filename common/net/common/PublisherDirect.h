#ifndef MIDAS_PUBLISHER_DIRECT_H
#define MIDAS_PUBLISHER_DIRECT_H

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
 * this will send msg directly. the deliver call will call a locking write to deliver data
 * no msg caching and will block if client cannot handle the amount
 * this can be used with small numbers of client without data loss
 */
template <typename Protocol, typename Derived, template <typename, typename> class PublisherBase>
class PublisherDirect : public PublisherBase<Protocol, Derived> {
public:
    typedef PublisherBase<Protocol, Derived> TPublisherBase;
    LockType::mutex mtx;

public:
    PublisherDirect(CChannel& channel, const string& cfg) : TPublisherBase(channel, cfg) {}

    ~PublisherDirect() {}

    /**
     * async send given msg to connected client, place msg in BufferManager then return
     */
    void deliver(const ConstBuffer& msg) {
        LockType::scoped_lock guard(mtx);
        if (this->closed()) return;

        try {
            if (boost::asio::write(this->skt, msg) != msg.size()) {
                MIDAS_LOG_ERROR("error write to socket " << this->get_name());
                this->close();
            } else {
                this->add_sent_msg_2_stat(msg.size());
            }
        } catch (boost::system::system_error& e) {
            MIDAS_LOG_ERROR("error write to socket " << this->get_name() << " : " << e.what());
            this->close();
        }
    }

    friend class PublisherBase<Protocol, Derived>;
};
}

#endif
