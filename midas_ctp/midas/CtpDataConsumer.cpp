#include <midas/MidasConfig.h>
#include "CtpDataConsumer.h"
#include "helper/CtpVisualHelper.h"

CtpDataConsumer::CtpDataConsumer(std::shared_ptr<CtpData> data) : data(data) {
    logRawData = Config::instance().get<bool>("ctp.logRawData", false);
    if (logRawData) {
        manager = make_shared<JournalManager>(data->dataDirectory + "/raw_msg_log", true);
    }
}

void CtpDataConsumer::data_callback1(MktDataPayload& payload) {
    ++receivedMsgCount;
    data->update(payload);

    if (logRawData) {
        ostringstream oss;
        oss << payload << '\n';
        manager->record(oss.str());
    }
}

void CtpDataConsumer::stats(ostream& os) {
    os << "CtpDataConsumer stats:" << '\n' << "msgs receive        = " << receivedMsgCount << '\n';
}

void CtpDataConsumer::flush() {
    if (logRawData) {
        manager->flush();
    }
}
