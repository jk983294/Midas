#ifndef MIDAS_CTP_SESSION_CONSUMER_H
#define MIDAS_CTP_SESSION_CONSUMER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include "MdTicker.h"
#include "model/CtpData.h"

class CtpSessionConsumer {
public:
    typedef std::shared_ptr<CtpSessionConsumer> SharedPtr;

    std::unordered_map<std::string, std::shared_ptr<MdTicker>>& subscriptions;
    long receivedMsgCount{0};

public:
    CtpSessionConsumer(std::unordered_map<std::string, std::shared_ptr<MdTicker>>& subscriptions_);

    void data_callback1(MktDataPayload& payload);

    void stats(ostream& os);
};

#endif
