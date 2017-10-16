#ifndef MIDAS_MY_PAYLOAD_H
#define MIDAS_MY_PAYLOAD_H

#include <net/disruptor/Payload.h>

class MyPayload : public midas::Payload {
public:
    MyPayload(const uint32_t bufferSize) : midas::Payload(bufferSize), m_data(new char[bufferSize]) {}

    char* str() { return m_data; }

private:
    char* m_data;
};

class MyData {
public:
    int data;

    MyData() {}
    MyData(int d) : data(d) {}
};

typedef midas::PayloadObject<MyData> MyDataPayload;

#endif
