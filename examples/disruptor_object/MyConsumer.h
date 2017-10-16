#ifndef MIDAS_MY_CONSUMER_H
#define MIDAS_MY_CONSUMER_H

#include "MyPayload.h"

class MyConsumer {
public:
    typedef std::shared_ptr<MyConsumer> SharedPtr;

    MyConsumer() : callbackCount(0) {}

    void data_callback1(MyPayload& payload) {
        ++callbackCount;
        total += payload.get_data().data;
        MIDAS_LOG_DEBUG("MyConsumer data_callback1 with count " << callbackCount);
    }

    uint32_t getCallbackCount() const { return callbackCount; }

public:
    std::atomic<uint32_t> callbackCount;
    int total{0};
};

class TwoStageMyConsumer : public MyConsumer {
public:
    typedef std::shared_ptr<TwoStageMyConsumer> SharedPtr;

    void data_callback1(MyPayload& payload) {
        MIDAS_LOG_DEBUG("TwoStageMyConsumer data_callback1 with count " << callbackCount);
    }

    void data_callback2(MyPayload& payload) {
        ++callbackCount;
        MIDAS_LOG_DEBUG("TwoStageMyConsumer data_callback2 with count " << callbackCount);
    }
};

#endif
