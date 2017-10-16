#ifndef MIDAS_CONSUMER_STRATEGY_H
#define MIDAS_CONSUMER_STRATEGY_H

#include <tbb/concurrent_queue.h>
#include <algorithm>
#include <atomic>
#include <boost/atomic.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>
#include <vector>
#include "utils/log/Log.h"

using namespace std;

namespace midas {

enum ConsumerStages { OneStage, TwoStage };

typedef boost::shared_ptr<boost::thread> ThreadPtr;

template <class Derived, typename PostCallbackType>
class ConsumerStrategy {
public:
    typedef typename boost::shared_ptr<ConsumerStrategy> SharedPtr;
    typedef PostCallbackType TPostCallback;
    typedef tbb::concurrent_queue<TPostCallback> TPostQueue;

    std::atomic<long> consumedTo;
    std::atomic<bool> isRunning;
    ThreadPtr consumerThreadPtr;
    TPostQueue& postQueueRef;

private:
    TPostCallback postCallback;
    const bool postHandler;
    boost::atomic<bool> isPaused;
    std::vector<ConsumerStrategy::SharedPtr> otherConsumers;

public:
    ConsumerStrategy(TPostQueue& postQueue_, bool postHandler_, const std::string& disruptorName)
        : consumedTo(-1), isRunning(true), postQueueRef(postQueue_), postHandler(postHandler_), isPaused(false) {}

    virtual ~ConsumerStrategy() {}

    long get_consumption_point() const { return consumedTo; }

    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void start(ConsumerBarrier& consumerBarrierPtr, Consumer& consumerRef) {
        impl().start<Consumer, Payload, ConsumerBarrier, Stages>(consumerBarrierPtr, consumerRef);
    }

    /**
     * overloaded function to start a consumer that uses the thread from the io_service instead of internal thread
     */
    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void start(ConsumerBarrier& consumerBarrierPtr, Consumer& consumerRef, boost::asio::io_service& io) {
        impl().start<Consumer, Payload, ConsumerBarrier, Stages>(consumerBarrierPtr, consumerRef, io);
    }

    template <class ConsumerBarrier>
    void stop(ConsumerBarrier consumerBarrierPtr) {
        isRunning = false;
        consumerBarrierPtr->interrupt();
        if (consumerThreadPtr) consumerThreadPtr->join();
    }

    template <class Consumer, class Payload, class ConsumerBarrier, ConsumerStages Stages>
    void run(Consumer& consumerRef, typename ConsumerBarrier::SharedPtr& consumerBarrierPtr){};

    static std::string print() { return Derived::print(); }

    void handle_post() {
        while (!postQueueRef.empty()) {
            if (postHandler) {
                while (!all_consumers_paused()) {  // wait here until other consumers have stopped
                    sched_yield();
                }

                if (postQueueRef.try_pop(postCallback)) {
                    postCallback();
                }
            } else {
                paused(true);  // inform the other thread that we've paused

                while (paused()) {  // Spin here until the other thread is done
                    sched_yield();
                }

                break;
            }
        }

        if (postHandler) {
            signal_other_threads();
        }
    }

    bool all_consumers_paused() const {
        uint32_t pausedConsumers =
            std::count_if(otherConsumers.begin(), otherConsumers.end(),
                          [](ConsumerStrategy::SharedPtr consumerPtr) { return consumerPtr->paused(); });

        return (pausedConsumers == otherConsumers.size());
    }

    // signal other consumer threads to continue
    void signal_other_threads() {
        std::for_each(otherConsumers.begin(), otherConsumers.end(),
                      [](ConsumerStrategy::SharedPtr consumerPtr) { consumerPtr->paused(false); });
    }

    void add_other_consumer(ConsumerStrategy::SharedPtr consumerStrategyPtr) {
        otherConsumers.push_back(consumerStrategyPtr);
    }

    void paused(bool paused) { isPaused = paused; }
    bool paused() const { return isPaused; }

private:
    const Derived& impl() const { return *static_cast<const Derived*>(this); }

    Derived& impl() { return *static_cast<Derived*>(this); }
};

template <class Consumer, class Payload, class RingBuffer, class ConsumerStrategy, ConsumerStages Stages>
class ConsumerHolder {
public:
    typedef boost::shared_ptr<ConsumerHolder> SharedPtr;
    typedef typename RingBuffer::TConsumerBarrier TConsumerBarrier;

    ConsumerHolder(typename TConsumerBarrier::SharedPtr consumerBarrierPtr, Consumer& consumerRef,
                   typename ConsumerStrategy::TPostQueue& postQueue, bool postHandler, const std::string& disruptorName)
        : consumerBarrierPtr(consumerBarrierPtr),
          consumerStrategyPtr(boost::make_shared<ConsumerStrategy>(postQueue, postHandler, disruptorName)) {
        consumerStrategyPtr->template start<Consumer, Payload, typename TConsumerBarrier::SharedPtr, Stages>(
            consumerBarrierPtr, consumerRef);
    }

    ConsumerHolder(typename TConsumerBarrier::SharedPtr consumerBarrierPtr, Consumer& consumerRef,
                   typename ConsumerStrategy::TPostQueue& postQueue, bool postHandler, const std::string& disruptorName,
                   boost::asio::io_service& io)
        : consumerBarrierPtr(consumerBarrierPtr),
          consumerStrategyPtr(boost::make_shared<ConsumerStrategy>(postQueue, postHandler, disruptorName)) {
        consumerStrategyPtr->template start<Consumer, Payload, typename TConsumerBarrier::SharedPtr, Stages>(
            consumerBarrierPtr, consumerRef, io);
    }

    ~ConsumerHolder() { stop(); }

    typename ConsumerStrategy::SharedPtr get_consumer_handler() { return consumerStrategyPtr; }

    void interrupt() { consumerBarrierPtr->interrupt(); }

    void stop() { consumerStrategyPtr->stop(consumerBarrierPtr); }

private:
    typename TConsumerBarrier::SharedPtr consumerBarrierPtr;
    typename ConsumerStrategy::SharedPtr consumerStrategyPtr;
};

template <class Consumer, class Payload, ConsumerStages stage>
class ConsumerDispatcher;

template <class Consumer, class Payload>
class ConsumerDispatcher<Consumer, Payload, OneStage> {
public:
    void dispatch(Consumer& consumerRef, Payload& payload) { consumerRef.data_callback1(payload); }
};

template <class Consumer, class Payload>
class ConsumerDispatcher<Consumer, Payload, TwoStage> {
public:
    void dispatch(Consumer& consumerRef, Payload& payload) { consumerRef.data_callback2(payload); }
};

/**
* this is used to define consumer callback stages
* take TwoStage as an example, it will call data_callback1 and then data_callback2 when a payload is received
*/
template <class Consumer, class Payload, class RingBuffer, class ConsumerStrategy, ConsumerStages Stages>
class NStageConsumer;

template <class Consumer, class Payload, class RingBuffer, class ConsumerStrategy>
class NStageConsumer<Consumer, Payload, RingBuffer, ConsumerStrategy, OneStage> {
public:
    typedef boost::shared_ptr<NStageConsumer<Consumer, Payload, RingBuffer, ConsumerStrategy, OneStage> > SharedPtr;
    typedef ConsumerHolder<Consumer, Payload, RingBuffer, ConsumerStrategy, OneStage> TOneStageHolder;

    typename TOneStageHolder::SharedPtr consumer1HolderPtr;

public:
    NStageConsumer(Consumer& consumerRef, typename RingBuffer::SharedPtr ringBufferPtr, const string& disruptorName_,
                   typename ConsumerStrategy::TPostQueue& postQueue) {
        consumer1HolderPtr = boost::make_shared<TOneStageHolder>(ringBufferPtr->create_consumer_barrier(), consumerRef,
                                                                 postQueue, true, disruptorName_ + ".c1");
        MIDAS_LOG_INFO(disruptorName_ << " consumer1 created.");
    }

    NStageConsumer(Consumer& consumerRef, typename RingBuffer::SharedPtr ringBufferPtr, const string& disruptorName_,
                   typename ConsumerStrategy::TPostQueue& postQueue, boost::asio::io_service& io) {
        consumer1HolderPtr = boost::make_shared<TOneStageHolder>(ringBufferPtr->create_consumer_barrier(), consumerRef,
                                                                 postQueue, true, disruptorName_ + ".c1", io);
        MIDAS_LOG_INFO(disruptorName_ << " consumer1 created.");
    }

    typename ConsumerStrategy::SharedPtr get_last_consumer() { return consumer1HolderPtr->get_consumer_handler(); }

    void interrupt() { consumer1HolderPtr->interrupt(); }

    void stop() { consumer1HolderPtr->stop(); }
};

template <class Consumer, class Payload, class RingBuffer, class ConsumerStrategy>
class NStageConsumer<Consumer, Payload, RingBuffer, ConsumerStrategy, TwoStage> {
public:
    typedef boost::shared_ptr<NStageConsumer<Consumer, Payload, RingBuffer, ConsumerStrategy, TwoStage> > SharedPtr;
    typedef ConsumerHolder<Consumer, Payload, RingBuffer, ConsumerStrategy, OneStage> TOneStageHolder;
    typedef ConsumerHolder<Consumer, Payload, RingBuffer, ConsumerStrategy, TwoStage> TTwoStageHolder;

    typename TOneStageHolder::SharedPtr consumer1HolderPtr;
    typename TTwoStageHolder::SharedPtr consumer2HolderPtr;

public:
    NStageConsumer(Consumer& consumerRef, typename RingBuffer::SharedPtr ringBufferPtr, const string& disruptorName_,
                   typename ConsumerStrategy::TPostQueue& postQueue) {
        consumer1HolderPtr = boost::make_shared<TOneStageHolder>(ringBufferPtr->create_consumer_barrier(), consumerRef,
                                                                 postQueue, false, disruptorName_ + ".c1");
        MIDAS_LOG_INFO(disruptorName_ << " consumer1 created.");

        consumer2HolderPtr = boost::make_shared<TTwoStageHolder>(
            ringBufferPtr->create_consumer_barrier((ConsumerStrategy*)consumer1HolderPtr->get_consumer_handler().get()),
            consumerRef, postQueue, true, disruptorName_ + ".c2");
        consumer2HolderPtr->get_consumer_handler()->add_other_consumer(consumer1HolderPtr->get_consumer_handler());
        MIDAS_LOG_INFO(disruptorName_ << " consumer2 created.");
    }

    typename ConsumerStrategy::SharedPtr get_last_consumer() { return consumer2HolderPtr->get_consumer_handler(); }

    void interrupt() {
        consumer1HolderPtr->interrupt();
        consumer2HolderPtr->interrupt();
    }

    void stop() {
        consumer1HolderPtr->stop();
        consumer2HolderPtr->stop();
    }
};
}

#endif
