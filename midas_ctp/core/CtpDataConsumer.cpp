#include "CtpDataConsumer.h"
#include "helper/CtpVisualHelper.h"

CtpDataConsumer::CtpDataConsumer(std::shared_ptr<CtpData> data) : data(data) {}

void CtpDataConsumer::data_callback1(MktDataPayload& payload) {
    ++receivedMsgCount;
    data->update(payload);
}

void CtpDataConsumer::stats(ostream& os) {
    os << "CtpDataConsumer stats:" << '\n' << "msgs recv        = " << receivedMsgCount << '\n';
}
