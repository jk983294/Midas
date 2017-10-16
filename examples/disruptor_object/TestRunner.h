#ifndef MIDAS_TEST_RUNNER_H
#define MIDAS_TEST_RUNNER_H

#include <net/disruptor/Disruptor.h>
#include "MyProducer.h"

using namespace midas;

class TestRunner {
public:
    typedef std::shared_ptr<TestRunner> SharedPtr;

    virtual ~TestRunner() {}

    virtual void add_producer(const std::string& name, const uint32_t iterations) = 0;
    virtual void add_consumer(const std::string& cfgPath) = 0;
    virtual uint32_t num_producers() const = 0;
    virtual void runTests(const uint32_t iterations) = 0;
};

template <class Consumer, class Payload, midas::ConsumerStages Stages>
class DisruptorTestRunner : public TestRunner {
public:
    void add_producer(const std::string& name, const uint32_t iterations) {
        m_producerStore.push_back(std::make_shared<MyProducer>(name, iterations));
    }

    void add_consumer(const std::string& cfgPath) {
        m_consumerPtr = std::make_shared<Consumer>();
        m_disruptorPtr =
            boost::make_shared<TMyDisruptor>("my_disruptor", cfgPath, m_producerStore, boost::ref(*m_consumerPtr));
    }

    uint32_t num_producers() const { return m_producerStore.size(); }

    void runTests(const uint32_t iterations) {
        if (iterations > 0) {
            // kick off producer(s)
            std::for_each(m_producerStore.begin(), m_producerStore.end(), boost::bind(&MyProducer::start, _1));

            // wait whilst consumer
            while (m_consumerPtr->getCallbackCount() < iterations) {
            }

            MIDAS_LOG_INFO("Total=" << m_consumerPtr->total);
        }
    }

public:
    typedef midas::Disruptor<MyProducer, Consumer, Payload, Stages> TMyDisruptor;

    std::vector<MyProducer::SharedPtr> m_producerStore;
    typename Consumer::SharedPtr m_consumerPtr;
    typename TMyDisruptor::SharedPtr m_disruptorPtr;
};

#endif
