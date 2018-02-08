#ifndef MIDAS_MY_CONSUMER_H
#define MIDAS_MY_CONSUMER_H

#include "MyPayload.h"

class MyConsumer {
public:
    typedef std::shared_ptr<MyConsumer> SharedPtr;

    MyConsumer() : callbackCount(0), c(0) {}
    virtual ~MyConsumer() = default;

    void data_callback1(midas::Payload& payload) {
        ++callbackCount;
        c = payload.buffer()[5];
        MIDAS_LOG_DEBUG("MyConsumer data_callback1 with count " << callbackCount);
    }

    uint32_t getCallbackCount() const { return callbackCount; }

protected:
    std::atomic<uint32_t> callbackCount;
    char c;
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
