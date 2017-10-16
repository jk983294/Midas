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

    void set_cursor(long seq) { return cursor.store(seq, std::memory_order::memory_order_release); }

public:
    class ConsumerTrackingProducerBarrier : public ProducerBarrier<Payload, ConsumerTrackingProducerBarrier> {
    private:
        const bool multiProducer;
        RingBuffer<Payload, ClaimStrategy, WaitStrategy, ConsumerStrategy>& ringBufferRef;
        const typename ConsumerStrategy::SharedPtr consumerPtr;
        typename WaitStrategy::SharedPtr waitStrategyPtr;
        long previousSize;

    public:
        ConsumerTrackingProducerBarrier(const bool multiProducer_,
                                        RingBuffer<Payload, ClaimStrategy, WaitStrategy, ConsumerStrategy>& ringBuffer_,
                                        const typename ConsumerStrategy::SharedPtr consumerPtr_,
                                        typename WaitStrategy::SharedPtr waitStrategyPtr_)
            : multiProducer(multiProducer_),
              ringBufferRef(ringBuffer_),
              consumerPtr(consumerPtr_),
              waitStrategyPtr(waitStrategyPtr_),
              previousSize(1) {}

        Payload& get_next_entry() {
            const long consumptionPoint = consumerPtr->get_consumption_point();
            const long nextSeqNo = ringBufferRef.claimStrategyPtr->increment_and_get();

            const long wrapPoint = nextSeqNo - ringBufferRef.ringSize;
            if (midas::is_unlikely_hint(wrapPoint > consumerPtr->get_consumption_point())) {
                bool loggedRingFullMsg = false;
                while (wrapPoint > consumerPtr->get_consumption_point()) {
                    if (!loggedRingFullMsg) {
                        MIDAS_LOG_WARNING(ringBufferRef.name << " Ring full detected");
                        loggedRingFullMsg = true;
                    }

                    sched_yield();
                }
            }

            Payload& entry = ringBufferRef.ring[nextSeqNo & ringBufferRef.bitMask];

            entry.set_sequence_value(nextSeqNo, consumptionPoint);
            return entry;
        }

        void publish_entry(Payload& entry, bool isValid = true) {
            entry.set_is_valid(isValid);

            if (multiProducer) {
                while ((entry.get_sequence_value() - 1) != ringBufferRef.get_cursor()) {
                    // Wait here until preceding producers have published their entries
                    sched_yield();
                }
            }

            ringBufferRef.set_cursor(entry.get_sequence_value());
            waitStrategyPtr->data_available();
        }
    };

    class ConsumerTrackingConsumerBarrier : public ConsumerBarrier<Payload, ConsumerTrackingConsumerBarrier> {
    public:
        typedef ProducerBarrier<Payload, ConsumerTrackingProducerBarrier> TProducerBarrier;
        typedef ConsumerBarrier<Payload, ConsumerTrackingConsumerBarrier> TConsumerBarrier;

    public:
        ConsumerTrackingConsumerBarrier(RingBuffer<Payload, ClaimStrategy, WaitStrategy, ConsumerStrategy>& ringBuffer_,
                                        const ConsumerStrategy* pConsumer_)
            : ringBufferRef(ringBuffer_), pConsumer(pConsumer_) {}

        Payload& get_entry(const long sequence) { return ringBufferRef.ring[sequence & ringBufferRef.bitMask]; }

        long wait_for(const long sequence, std::atomic<bool>& interruptRef,
                      typename ConsumerStrategy::TPostQueue& postQueue) {
            return ringBufferRef.waitStrategyPtr->wait_for(sequence, ringBufferRef, pConsumer, interruptRef, postQueue);
        }

        void interrupt() { ringBufferRef.waitStrategyPtr->data_available(); }

    private:
        RingBuffer<Payload, ClaimStrategy, WaitStrategy, ConsumerStrategy>& ringBufferRef;
        const ConsumerStrategy* pConsumer;
    };

    typedef ProducerBarrier<Payload, ConsumerTrackingProducerBarrier> TProducerBarrier;
    typedef ConsumerBarrier<Payload, ConsumerTrackingConsumerBarrier> TConsumerBarrier;

    typename TProducerBarrier::SharedPtr create_producer_barrier(
        bool multiProducer, const typename ConsumerStrategy::SharedPtr consumerPtr) {
        return boost::make_shared<ConsumerTrackingProducerBarrier>(multiProducer, *this, consumerPtr, waitStrategyPtr);
    }

    typename TConsumerBarrier::SharedPtr create_consumer_barrier(const ConsumerStrategy* pConsumerToTrack = nullptr) {
        return boost::make_shared<ConsumerTrackingConsumerBarrier>(*this, pConsumerToTrack);
    }
};
}

#endif
