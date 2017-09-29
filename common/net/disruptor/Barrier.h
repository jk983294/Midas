#ifndef MIDAS_BARRIER_H
#define MIDAS_BARRIER_H

#include <atomic>
#include <boost/shared_ptr.hpp>

namespace midas {

template <typename Payload, class Derived>
class ProducerBarrier {
public:
    typedef typename boost::shared_ptr<ProducerBarrier> SharedPtr;

public:
    virtual ~ProducerBarrier() {}

    Payload& get_next_entry() { return impl().get_next_entry(); }

    void publish_entry(Payload& entry, bool isValid) { impl().publish_entry(entry, isValid); }

private:
    Derived& impl() { return *static_cast<Derived*>(this); }
};

template <typename Payload, class Derived>
class ConsumerBarrier {
public:
    typedef typename boost::shared_ptr<ConsumerBarrier> SharedPtr;

public:
    virtual ~ConsumerBarrier() {}

    Payload& get_entry(const long sequence) { return impl().get_entry(sequence); }

    template <typename TPostQueue>
    long wait_for(const long sequence, std::atomic<bool>& interruptRef, TPostQueue& queue) {
        return impl().wait_for(sequence, interruptRef, queue);
    }

    void interrupt() { impl().interrupt(); }

private:
    Derived& impl() { return *static_cast<Derived*>(this); }
};
}

#endif
