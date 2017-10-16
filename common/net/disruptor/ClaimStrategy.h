#ifndef MIDAS_CLAIM_STRATEGY_H
#define MIDAS_CLAIM_STRATEGY_H

#include <boost/shared_ptr.hpp>

#include <atomic>
#include <string>
#include <type_traits>

using namespace std;

namespace midas {

/**
 * ClaimStrategy interface
 */
template <class Derived>
class ClaimStrategy {
public:
    typedef boost::shared_ptr<ClaimStrategy<Derived> > SharedPtr;

    virtual ~ClaimStrategy() {}

    long increment_and_get() { return impl().increment_and_get(); }

    void set_sequence(const long sequence) { impl().set_sequence(sequence); }

    std::string print() { return impl().print(); }

private:
    Derived& impl() { return *static_cast<Derived*>(this); }
};

/**
 * implementation for ClaimStrategy interface:
 * SingleThreadedStrategy
 * MultiThreadedStrategy
 */

class SingleThreadedStrategy : public ClaimStrategy<SingleThreadedStrategy> {
public:
    typedef std::false_type MultiThreaded;

    SingleThreadedStrategy() {}

    long increment_and_get() { return ++sequence; }

    void set_sequence(const long& sequence_) { sequence = sequence_; }

    std::string print() const { return "SINGLE_THREADED"; }

private:
    long sequence{-1};
};

class MultiThreadedStrategy : public ClaimStrategy<MultiThreadedStrategy> {
public:
    typedef std::true_type MultiThreaded;

    MultiThreadedStrategy() : sequence(-1) {}

    long increment_and_get() { return sequence.fetch_add(1, std::memory_order::memory_order_release) + 1; }

    void set_sequence(const long& sequence_) { sequence = sequence_; }

    std::string print() const { return "MULTI_THREADED"; }

private:
    std::atomic<long> sequence;
};
}

#endif
