#ifndef MIDAS_WAIT_STRATEGY_H
#define MIDAS_WAIT_STRATEGY_H

#include <atomic>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/pthread/condition_variable_fwd.hpp>
#include <string>
#include "ConsumerStrategy.h"
#include "utils/log/Log.h"

using namespace std;

namespace midas {

// forward declaration
template <typename Payload, class ClaimStrategy, class WaitStrategy, class ConsumerStrategy>
class RingBuffer;

template <class Derived>
class WaitStrategy {
public:
    typedef typename boost::shared_ptr<WaitStrategy> SharedPtr;

    template <class RingBuffer, class ConsumerStrategy>
    long wait_for(const long sequence, RingBuffer& ringBuffer, const ConsumerStrategy* pConsumer,
                  std::atomic<bool>& runningRef, typename ConsumerStrategy::TPostQueue& postQueue) {
        return impl().wait_for(sequence, ringBuffer, pConsumer, runningRef, postQueue);
    }

    void data_available() { impl().data_available(); }

    std::string print() { return impl().print(); }

private:
    Derived& impl() { return *static_cast<Derived*>(this); }
};

/**
 * this should only be used if the number of Event Handler threads is smaller than
 * the number of physical cores on the box. E.g. hyper-threading should be disabled.
 */
class BusySpinWaitStrategy : public WaitStrategy<BusySpinWaitStrategy> {
public:
    template <class RingBuffer, class ConsumerStrategy>
    long wait_for(const long sequence, RingBuffer& ringBuffer, const ConsumerStrategy* pConsumer,
                  std::atomic<bool>& runningRef, typename ConsumerStrategy::TPostQueue& postQueue) {
        long availableSequence = 0;

        if (pConsumer) {
            while (((availableSequence = pConsumer->get_consumption_point()) < sequence) &&
                   runningRef.load(std::memory_order::memory_order_acquire) && postQueue.empty()) {
                __asm__ __volatile__("pause");
            }
        } else {
            while (((availableSequence = ringBuffer.get_cursor()) < sequence) &&
                   runningRef.load(std::memory_order::memory_order_acquire) && postQueue.empty()) {
                __asm__ __volatile__("pause");
            }
        }
        return availableSequence;
    }

    void data_available() {}

    std::string print() const { return "BUSY_SPIN"; }
};

/**
 * recommended when need very high performance
 * and the number of Event Handler threads is less than the total number of logical cores,
 * e.g. you have hyper-threading enabled.
 */
class YieldWaitStrategy : public WaitStrategy<YieldWaitStrategy> {
public:
    template <class RingBuffer, class ConsumerStrategy>
    long wait_for(const long sequence, RingBuffer& ringBuffer, const ConsumerStrategy* pConsumer,
                  std::atomic<bool>& runningRef, typename ConsumerStrategy::TPostQueue& postQueue) {
        long availableSequence = 0;

        if (pConsumer) {
            while (((availableSequence = pConsumer->get_consumption_point()) < sequence) && runningRef &&
                   postQueue.empty()) {
                sched_yield();
            }
        } else {
            while (((availableSequence = ringBuffer.get_cursor()) < sequence) && runningRef && postQueue.empty()) {
                sched_yield();
            }
        }
        return availableSequence;
    }

    void data_available() {}

    std::string print() const { return "YIELD"; }
};

/**
 * the slowest of the available wait strategies,
 * but it is the most conservative with the respect to CPU usage and
 * will give the most consistent behaviour across the widest variety of deployment options
 */
class BlockWaitStrategy : public WaitStrategy<BlockWaitStrategy> {
public:
    BlockWaitStrategy() {}

    template <class RingBuffer, class ConsumerStrategy>
    long wait_for(const long sequence, RingBuffer& ringBuffer, const ConsumerStrategy* pConsumer,
                  std::atomic<bool>& runningRef, typename ConsumerStrategy::TPostQueue& postQueue) {
        long availableSequence = 0;

        if (pConsumer) {
            while (((availableSequence = pConsumer->get_consumption_point()) < sequence) && runningRef &&
                   postQueue.empty()) {
                boost::unique_lock<boost::mutex> lock(dataAvailableMutex);
                while (!dataAvailable) {
                    dataAvailableCondition.wait(lock);  // BLOCK here!
                }
            }
        } else {
            while (((availableSequence = ringBuffer.get_cursor()) < sequence) && runningRef && postQueue.empty()) {
                boost::unique_lock<boost::mutex> lock(dataAvailableMutex);
                while (!dataAvailable) {
                    dataAvailableCondition.wait(lock);  // BLOCK here!
                }

                dataAvailable = false;
            }
        }
        return availableSequence;
    }

    void data_available() {
        {
            boost::lock_guard<boost::mutex> lock(dataAvailableMutex);
            dataAvailable = true;
        }
        dataAvailableCondition.notify_all();
    }

    std::string print() const { return "BLOCK"; }

private:
    bool dataAvailable{false};
    boost::mutex dataAvailableMutex;
    boost::condition_variable dataAvailableCondition;
};
}

#endif
