#include <midas/MidasConfig.h>
#include "CtpSessionConsumer.h"
#include "helper/CtpVisualHelper.h"

CtpSessionConsumer::CtpSessionConsumer(std::unordered_map<std::string, std::shared_ptr<MdTicker>>& subscriptions_)
    : subscriptions(subscriptions_) {}

void CtpSessionConsumer::data_callback1(MktDataPayload& payload) {
    ++receivedMsgCount;

    const CThostFtdcDepthMarketDataField& field = payload.get_data();
    string symbol{field.InstrumentID};
    auto itr = subscriptions.find(symbol);
    if (itr != subscriptions.end()) {
        MdTicker& ticker = *itr->second;
        int64_t askPrice = static_cast<int64_t>(field.AskPrice1);
        uint64_t askSize = static_cast<uint64_t>(field.AskVolume1);
        int64_t bidPrice = static_cast<int64_t>(field.BidPrice1);
        uint64_t bidSize = static_cast<uint64_t>(field.BidVolume1);
        ticker.update_best_ask_price_level(askPrice, askSize, 0, payload.rcvt);
        ticker.update_best_bid_price_level(bidPrice, bidSize, 0, payload.rcvt);
        ticker.process_book_refresh_L1();
    }
    MIDAS_LOG_DEBUG("recv: " << receivedMsgCount << " " << payload.get_data());
}

void CtpSessionConsumer::stats(ostream& os) {
    os << "CtpSessionConsumer stats:" << '\n' << "msgs receive        = " << receivedMsgCount << '\n';
}
