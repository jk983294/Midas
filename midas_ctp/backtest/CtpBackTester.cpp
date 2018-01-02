#include "CtpBackTester.h"
#include "helper/DataLoader.h"

CtpBackTester::CtpBackTester(int argc, char** argv) : MidasProcessBase(argc, argv) {
    data = make_shared<CtpData>();
    init_admin();
}

CtpBackTester::~CtpBackTester() {}

void CtpBackTester::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        throw MidasException();
    }

    if (dataDirectory.empty()) return;

    load_test_data(dataDirectory);
}

void CtpBackTester::load_test_data(const string& dataPath) {
    DataLoader dataLoader;
    dataLoader.load(dataPath);

    for (auto& item : dataLoader.instrument2candle) {
        TradeSessions* pts = data->tradeStatusManager.get_session(item.first);
        if (pts) {
            CtpInstrument instrument(item.first, *pts);
            instrument.load_historic_candle(item.second, CandleScale::Minute1);
            data->instruments.insert({item.first, instrument});
        }
    }
}

void CtpBackTester::app_stop() { MidasProcessBase::app_stop(); }
