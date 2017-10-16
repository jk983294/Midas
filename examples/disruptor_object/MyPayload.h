#ifndef MIDAS_MY_PAYLOAD_H
#define MIDAS_MY_PAYLOAD_H

#include <net/disruptor/Payload.h>

class MyData {
public:
    int data;

    MyData() {}
    MyData(int d) : data(d) {}
};

typedef midas::PayloadObject<MyData> MyPayload;

#endif
