#ifndef MIDAS_CTP_DATA_LOG_CONSUMER_H
#define MIDAS_CTP_DATA_LOG_CONSUMER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <io/JournalManager.h>
#include "helper/CtpVisualHelper.h"
#include "model/CtpData.h"

class CtpDataLogConsumer {
public:
    typedef std::shared_ptr<CtpDataLogConsumer> SharedPtr;

    long receivedMsgCount{0};

    std::shared_ptr<CtpData> data;

    std::shared_ptr<JournalManager> manager;

public:
    CtpDataLogConsumer(std::shared_ptr<CtpData> data) : data(data) {
        manager = make_shared<JournalManager>(data->dataDirectory + "/raw_msg_log", true);
    }

    void data_callback1(MktDataPayload& payload) {
        ++receivedMsgCount;
        ostringstream oss;
        oss << payload << '\n';
        manager->record(oss.str());
    }

    void stats(ostream& os) {
        os << "CtpDataConsumer stats:" << '\n' << "msgs recv        = " << receivedMsgCount << '\n';
    }

    void flush() { manager->flush(); }
};

#endif
