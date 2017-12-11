#ifndef MIDAS_MY_PROCESS_H
#define MIDAS_MY_PROCESS_H

#include <utils/TimeUtils.h>
#include "MyConsumer.h"
#include "MyPayload.h"
#include "TestRunner.h"
#include "process/MidasProcessBase.h"

using namespace midas;

class MyProcess : public MidasProcessBase {
public:
    MyProcess(int argc, char* argv[]) : MidasProcessBase(argc, argv) {}

    ~MyProcess() {}

    void app_start() {
        const std::string cfgPath = "test.disruptor";
        string producerName = "producer";
        m_iterations = Config::instance().get<uint32_t>("test.perf_test_iterations", 100000);
        int producerNumber = Config::instance().get<int>("test.producer_number", 1);
        int consumerNumber = Config::instance().get<int>("test.consumer_number", 1);

        if (consumerNumber == 1)
            m_testRunnerPtr =
                std::make_shared<DisruptorTestRunner<MyConsumer, midas::Payload, midas::ConsumerStages::OneStage> >();
        else
            m_testRunnerPtr = std::make_shared<
                DisruptorTestRunner<TwoStageMyConsumer, MyPayload, midas::ConsumerStages::TwoStage> >();

        for (int i = 0; i < producerNumber; ++i) {
            m_testRunnerPtr->add_producer(producerName, m_iterations);
        }

        m_testRunnerPtr->add_consumer(cfgPath);
    }

    void app_ready() {
        if (m_iterations > 0) {
            Timer latency;
            const uint32_t totalIterations = m_iterations * m_testRunnerPtr->num_producers();
            MIDAS_LOG_INFO("TEST RUNS=" << totalIterations);
            MIDAS_LOG_WARNING("**** TEST START ****");

            latency.start();
            m_testRunnerPtr->runTests(totalIterations);
            latency.stop();

            MIDAS_LOG_WARNING("**** TEST COMPLETE ****");
            MIDAS_LOG_INFO("Duration=" << latency.elapsed() << " ns");
            MIDAS_LOG_INFO("Mean Latency=" << (latency.elapsed() / totalIterations) << " ns/msg");

            m_testRunnerPtr.reset();
        } else {
            MIDAS_LOG_WARNING("Zero test iterations, post test assumed");
        }
    }

private:
    TestRunner::SharedPtr m_testRunnerPtr;
    uint32_t m_iterations;
};

#endif
