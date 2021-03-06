#ifndef MIDAS_SEQUENTIAL_CONSUMER_STRATEGY_H
#define MIDAS_SEQUENTIAL_CONSUMER_STRATEGY_H

#include <atomic>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include "Barrier.h"
#include "ConsumerStrategy.h"
#include "utils/MidasUtils.h"

using namespace std;

namespace midas {

template <typename PostCallbackType = boost::function<void(void)>>
class SequentialConsumerStrategy
    : public ConsumerStrategy<SequentialConsumerStrategy<PostCallbackType>, PostCallbackType> {
public:
    typedef ConsumerStrategy<SequentialConsumerStrategy<PostCallbackType>, PostCallbackType> BaseType;

    using BaseType::consumedTo;
    using BaseType::isRunning;
    using BaseType::postQueueRef;
    using BaseType::consumerThreadPtr;
    using BaseType::handle_post;
    using typename BaseType::TPostCallback;
    using typename BaseType::TPostQueue;

    SequentialConsumerStrategy(TPostQueue& postQueue_, bool postHandler_, const std::string& disruptorName_)
        : BaseType(postQueue_, postHandler_, disruptorName_) {}

    virtual ~SequentialConsumerStrategy() {
        if (consumerThreadPtr) consumerThreadPtr->join();
    }

    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void run(Consumer& consumerRef, ConsumerBarrier& consumerBarrierPtr) {
        ConsumerDispatcher<Consumer, Payload, Stages> dispatcher;
        long availableSequence = 0;
        long nextSequence = 0;
        Payload* entry = nullptr;

        while (isRunning) {
            availableSequence = consumerBarrierPtr->wait_for(nextSequence, isRunning, postQueueRef);

            if (midas::is_unlikely_hint(!postQueueRef.empty())) {
                handle_post();
            }

            while (nextSequence <= availableSequence) {
                entry = &(consumerBarrierPtr->get_entry(nextSequence));
                ++nextSequence;

                if (is_unlikely_hint(!entry->is_valid())) {
                    continue;  // skip this payload
                }

                entry->set_has_more_data(nextSequence <= availableSequence);

                dispatcher.dispatch(consumerRef, *entry);
            }

            if (midas::is_likely_hint(entry)) {
                consumedTo.store(entry->get_sequence_value(), std::memory_order::memory_order_release);
            }
        }
    }

    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void start(ConsumerBarrier& consumerBarrierPtr, Consumer& consumerRef) {
        consumerThreadPtr = boost::make_shared<boost::thread>(boost::bind(
            boost::bind(&SequentialConsumerStrategy<PostCallbackType>::run<Consumer, Payload, ConsumerBarrier, Stages>,
                        this, boost::ref(consumerRef), consumerBarrierPtr)));
    }

    /**
     * overloaded one to support the use of the io_service thread instead of the disruptor spawning one
     * useful when we are doing asynchronous disruptor operations based on asio event model
     * this consumer will take over the thread and in most cases will not return control to the io_service
     * poll() should be called within the consumer operation by the consumer thread
     */
    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void start(ConsumerBarrier& consumerBarrierPtr, Consumer& consumerRef, boost::asio::io_service& io) {
        io.post(boost::bind(
            boost::bind(&SequentialConsumerStrategy<PostCallbackType>::run<Consumer, Payload, ConsumerBarrier, Stages>,
                        this, boost::ref(consumerRef), consumerBarrierPtr)));
    }

    static std::string print() { return "SEQUENTIAL"; }
};
}

#endif
