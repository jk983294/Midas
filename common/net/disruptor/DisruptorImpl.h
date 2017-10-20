#ifndef MIDAS_DISRUPTOR_IMPL_H
#define MIDAS_DISRUPTOR_IMPL_H

#include <tbb/concurrent_queue.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <type_traits>
#include <vector>
#include "ClaimStrategy.h"
#include "Payload.h"
#include "RingBuffer.h"
#include "WaitStrategy.h"

using namespace std;

namespace midas {

template <typename ConsumerStrategy>
class DisruptorImplBase {
public:
    typedef boost::shared_ptr<DisruptorImplBase> SharedPtr;

    virtual ~DisruptorImplBase(){};
    virtual std::string name() const = 0;
    virtual void post(const typename ConsumerStrategy::TPostCallback& cb) = 0;
    virtual void stop() = 0;
    virtual void stats(ostream& os) = 0;
};

template <class Producer, class Consumer, class Payload, ConsumerStages Stages, class ClaimStrategy, class WaitStrategy,
          class ConsumerStrategy>
class DisruptorImpl : public DisruptorImplBase<ConsumerStrategy> {
public:
    typedef RingBuffer<Payload, ClaimStrategy, WaitStrategy, ConsumerStrategy> TRingBuffer;
    typedef NStageConsumer<Consumer, Payload, TRingBuffer, ConsumerStrategy, Stages> TStageConsumer;
    typedef std::vector<typename Producer::SharedPtr> TProducerStore;

    typedef typename ConsumerStrategy::TPostCallback TPostCallback;
    typedef typename ConsumerStrategy::TPostQueue TPostQueue;

private:
    const std::string disruptorName;
    Consumer& consumerRef;
    typename TRingBuffer::SharedPtr ringBufferPtr;
    typename TStageConsumer::SharedPtr consumerStagesPtr;
    typename TRingBuffer::TProducerBarrier::SharedPtr producerBarrierPtr;
    TPostQueue postQueue;
    long receivedMsgCount{0};

public:
    DisruptorImpl(std::string disruptorName_, const uint32_t maxMsgSize_, const uint32_t ringSizeExponent_,
                  TProducerStore& producers, Consumer& consumerRef_)
        : disruptorName(disruptorName_),
          consumerRef(consumerRef_),
          ringBufferPtr(new TRingBuffer(maxMsgSize_, (uint32_t)pow(2, ringSizeExponent_), disruptorName_)) {
        consumerStagesPtr = boost::make_shared<TStageConsumer>(consumerRef_, ringBufferPtr, disruptorName_, postQueue);

        setup_producers(producers.size() > 1, producers);
    }

    ~DisruptorImpl() {
        consumerStagesPtr->stop();
        consumerStagesPtr.reset();
    }

    // Dealing with default payload type
    template <typename PL = Payload>
    typename std::enable_if<std::is_base_of<typename midas::Payload, PL>::value, void>::type setup_producers(
        const bool multiProducer, TProducerStore& producers) {
        producerBarrierPtr =
            ringBufferPtr->create_producer_barrier(multiProducer, consumerStagesPtr->get_last_consumer());

        for_each(producers.begin(), producers.end(), [this](typename Producer::SharedPtr producerPtr) {
            producerPtr->register_data_callback(
                [this](const char* data, size_t size, uint64_t rcvt, int64_t id) -> std::size_t {
                    ++receivedMsgCount;
                    Payload& entry = producerBarrierPtr->get_next_entry();
                    bool isValid = entry.set_value(data, size, rcvt, id);
                    producerBarrierPtr->publish_entry(entry, isValid);
                    return size;
                });
        });
    }

    // Dealing with generic payload type
    template <typename PL = Payload>
    typename std::enable_if<!std::is_base_of<typename midas::Payload, PL>::value, void>::type setup_producers(
        const bool multiProducer, TProducerStore& producers) {
        producerBarrierPtr =
            ringBufferPtr->create_producer_barrier(multiProducer, consumerStagesPtr->get_last_consumer());

        for_each(producers.begin(), producers.end(), [this](typename Producer::SharedPtr producerPtr) {
            producerPtr->register_data_callback(
                [this](const typename Payload::ElementType& data, uint64_t rcvt, int64_t id) -> std::size_t {
                    ++receivedMsgCount;
                    Payload& entry = producerBarrierPtr->get_next_entry();
                    bool isValid = entry.set_value(data, rcvt, id);
                    producerBarrierPtr->publish_entry(entry, isValid);
                    return 0;
                });
        });
    }

    std::string name() const { return disruptorName; }

    void post(const TPostCallback& cb) {
        postQueue.push(cb);
        consumerStagesPtr->interrupt();
    }

    void stop() override { consumerStagesPtr->stop(); }

    void stats(ostream& os) { os << "Disruptor stats:" << endl << "msgs recv        = " << receivedMsgCount << endl; }
};
}

#endif
