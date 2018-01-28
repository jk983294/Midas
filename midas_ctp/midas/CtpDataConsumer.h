#ifndef MIDAS_CTP_DATA_CONSUMER_H
#define MIDAS_CTP_DATA_CONSUMER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <io/JournalManager.h>
#include "model/CtpData.h"

class CtpDataConsumer {
public:
    typedef std::shared_ptr<CtpDataConsumer> SharedPtr;

    bool logRawData{false};

    long receivedMsgCount{0};

    std::shared_ptr<CtpData> data;

    std::shared_ptr<JournalManager> manager;

public:
    CtpDataConsumer(std::shared_ptr<CtpData> data);

    void data_callback1(MktDataPayload& payload);

    void stats(ostream& os);

    void flush();
};

#endif
