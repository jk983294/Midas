#ifndef MIDAS_RINGBUFFER_H
#define MIDAS_RINGBUFFER_H

#include <atomic>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdlib>
#include <string>
#include <vector>
#include "Barrier.h"
#include "utils/ConvertHelper.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {
template <typename Payload, typename ClaimStrategy, typename WaitStrategy, typename ConsumerStrategy>
class RingBuffer {
public:
    typedef boost::shared_ptr<RingBuffer> SharedPtr;

    // one cache line
    std::atomic<long> cursor;
    const long bitMask;
    const string name;
    const size_t ringSize;
    uint64_t padding2preventFalseSharing[4];

    vector<Payload> ring;
    typename ClaimStrategy::SharedPtr claimStrategyPtr;
    typename WaitStrategy::SharedPtr waitStrategyPtr;

public:
    RingBuffer(const uint32_t maxMsgSize, const size_t ringSize_, string name_)
        : cursor(-1),
          bitMask(ringSize_ - 1),
          name(name_),
          ringSize(ringSize_),
          ring(ringSize_, Payload(maxMsgSize)),
          claimStrategyPtr(boost::make_shared<ClaimStrategy>()),
          waitStrategyPtr(boost::make_shared<WaitStrategy>()) {
        MIDAS_LOG_INFO(name << " maxMsgSize: " << maxMsgSize);
        MIDAS_LOG_INFO(name << " ringSize: " << ringSize);
        MIDAS_LOG_INFO(name << " ClaimStrategy: " << claimStrategyPtr->print());
        MIDAS_LOG_INFO(name << " WaitStrategy: " << waitStrategyPtr->print());
        MIDAS_LOG_INFO(name << " ConsumerStrategy: " << ConsumerStrategy::print());
    }

    long get_cursor() const { return cursor.load(std::memory_order::memory_order_acquire); }

    void set_cursor(long seq) const { return cursor.store(seq, std::memory_order::memory_order_release); }
};
}

#endif
