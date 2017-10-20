#include "../helper/CtpVisualHelper.h"
#include "CtpDataConsumer.h"

CtpDataConsumer::CtpDataConsumer(std::shared_ptr<CtpData> data) : data(data) {}

void CtpDataConsumer::data_callback1(MktDataPayload& payload) {
    //    MIDAS_LOG_DEBUG("OnRtnDepthMarketData " << payload.get_data());
    ++receivedMsgCount;
    data->books.update(payload);
}

void CtpDataConsumer::stats(ostream& os) {
    os << "CtpDataConsumer stats:" << endl << "msgs recv        = " << receivedMsgCount << endl;
}
