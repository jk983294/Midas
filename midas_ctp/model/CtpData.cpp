#include "CtpData.h"

void CtpData::init_all_instruments() {
    for (auto itr = instrumentInfo.begin(); itr != instrumentInfo.end(); ++itr) {
        TradeSessions *pts = tradeStatusManager.get_session(itr->first);
        if (pts) {
            instruments.insert({itr->first, CtpInstrument{itr->first, *pts}});
        }
    }
}

void CtpData::stream(ostream &os, const string &instrument, bool isImage) {
    auto itr = instruments.find(instrument);
    if (itr == instruments.end()) {
        os << "no book found for " << instrument;
    }

    if (isImage)
        itr->second.image_stream(os);
    else
        itr->second.book_stream(os);
}

bool CtpData::update(const MktDataPayload &tick) {
    string instrument{tick.get_data().InstrumentID};
    auto itr = instruments.find(instrument);
    if (itr == instruments.end()) return false;

    itr->second.update_tick(tick);
    return true;
}
