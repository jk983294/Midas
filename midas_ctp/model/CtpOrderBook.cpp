#include <iomanip>
#include "CtpOrderBook.h"

CtpBookEntry::CtpBookEntry(const string &instrument) : instrument(instrument) {}

inline static bool is_volume_abnormal(int v) { return v <= 0 || v > 100000000; }

void CtpBookEntry::book_stream(ostream &os) {
    const static int defaultPrice = -99999;
    os << "Price          |         Volume" << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume5) ? defaultPrice : image.AskPrice5) << "|" << right
       << setw(15) << image.AskVolume5 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume4) ? defaultPrice : image.AskPrice4) << "|" << right
       << setw(15) << image.AskVolume4 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume3) ? defaultPrice : image.AskPrice3) << "|" << right
       << setw(15) << image.AskVolume3 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume2) ? defaultPrice : image.AskPrice2) << "|" << right
       << setw(15) << image.AskVolume2 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.AskVolume1) ? defaultPrice : image.AskPrice1) << "|" << right
       << setw(15) << image.AskVolume1 << '\n'
       << "---------------|---------------" << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume1) ? defaultPrice : image.BidPrice1) << "|" << right
       << setw(15) << image.BidVolume1 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume2) ? defaultPrice : image.BidPrice2) << "|" << right
       << setw(15) << image.BidVolume2 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume3) ? defaultPrice : image.BidPrice3) << "|" << right
       << setw(15) << image.BidVolume3 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume4) ? defaultPrice : image.BidPrice4) << "|" << right
       << setw(15) << image.BidVolume4 << '\n'
       << left << setw(15) << (is_volume_abnormal(image.BidVolume5) ? defaultPrice : image.BidPrice5) << "|" << right
       << setw(15) << image.BidVolume5;
}

void CtpBookEntry::image_stream(ostream &os) {
    os << left << setw(15) << "e " << right << setw(15) << image.ExchangeID << '\n'
       << left << setw(15) << "id " << right << setw(15) << image.InstrumentID << '\n'
       << left << setw(15) << "tp " << right << setw(15) << image.LastPrice << '\n'
       << left << setw(15) << "lsp " << right << setw(15) << image.PreSettlementPrice << '\n'
       << left << setw(15) << "lccp " << right << setw(15) << image.PreClosePrice << '\n'
       << left << setw(15) << "lois " << right << setw(15) << image.PreOpenInterest << '\n'
       << left << setw(15) << "op " << right << setw(15) << image.OpenPrice << '\n'
       << left << setw(15) << "hp " << right << setw(15) << image.HighestPrice << '\n'
       << left << setw(15) << "lp " << right << setw(15) << image.LowestPrice << '\n'
       << left << setw(15) << "ts " << right << setw(15) << image.Volume << '\n'
       << left << setw(15) << "to " << right << setw(15) << image.Turnover << '\n'
       << left << setw(15) << "ois " << right << setw(15) << image.OpenInterest << '\n'
       << left << setw(15) << "cp " << right << setw(15) << image.ClosePrice << '\n'
       << left << setw(15) << "sp " << right << setw(15) << image.SettlementPrice << '\n'
       << left << setw(15) << "ulp " << right << setw(15) << image.UpperLimitPrice << '\n'
       << left << setw(15) << "llp " << right << setw(15) << image.LowerLimitPrice << '\n'
       << left << setw(15) << "pd " << right << setw(15) << image.PreDelta << '\n'
       << left << setw(15) << "cd " << right << setw(15) << image.CurrDelta << '\n'
       << left << setw(15) << "ut " << right << setw(15) << image.UpdateTime << '\n'
       << left << setw(15) << "um " << right << setw(15) << image.UpdateMillisec << '\n'
       << left << setw(15) << "bbp1 " << right << setw(15) << image.BidPrice1 << '\n'
       << left << setw(15) << "bbs1 " << right << setw(15) << image.BidVolume1 << '\n'
       << left << setw(15) << "bbp2 " << right << setw(15) << image.BidPrice2 << '\n'
       << left << setw(15) << "bbs2 " << right << setw(15) << image.BidVolume2 << '\n'
       << left << setw(15) << "bbp3 " << right << setw(15) << image.BidPrice3 << '\n'
       << left << setw(15) << "bbs3 " << right << setw(15) << image.BidVolume3 << '\n'
       << left << setw(15) << "bbp4 " << right << setw(15) << image.BidPrice4 << '\n'
       << left << setw(15) << "bbs4 " << right << setw(15) << image.BidVolume4 << '\n'
       << left << setw(15) << "bbp5 " << right << setw(15) << image.BidPrice5 << '\n'
       << left << setw(15) << "bbs5 " << right << setw(15) << image.BidVolume5 << '\n'
       << left << setw(15) << "bap1 " << right << setw(15) << image.AskPrice1 << '\n'
       << left << setw(15) << "bas1 " << right << setw(15) << image.AskVolume1 << '\n'
       << left << setw(15) << "bap2 " << right << setw(15) << image.AskPrice2 << '\n'
       << left << setw(15) << "bas2 " << right << setw(15) << image.AskVolume2 << '\n'
       << left << setw(15) << "bap3 " << right << setw(15) << image.AskPrice3 << '\n'
       << left << setw(15) << "bas3 " << right << setw(15) << image.AskVolume3 << '\n'
       << left << setw(15) << "bap4 " << right << setw(15) << image.AskPrice4 << '\n'
       << left << setw(15) << "bas4 " << right << setw(15) << image.AskVolume4 << '\n'
       << left << setw(15) << "bap5 " << right << setw(15) << image.AskPrice5 << '\n'
       << left << setw(15) << "bas5 " << right << setw(15) << image.AskVolume5 << '\n'
       << left << setw(15) << "avgp " << right << setw(15) << image.AveragePrice << '\n'
       << left << setw(15) << "ad " << right << setw(15) << image.ActionDay << '\n'
       << left << setw(15) << "updateCount " << right << setw(15) << updateCount << '\n';
}

void CtpBookEntry::update_image(const CThostFtdcDepthMarketDataField &img) {
    image = img;
    ++updateCount;
}

void CtpBooks::init_all_instruments(const map<string, CThostFtdcInstrumentField> &instruments) {
    for (auto itr = instruments.begin(); itr != instruments.end(); ++itr) {
        entries.insert(make_pair(itr->first, CtpBookEntry(itr->first)));
    }
}

void CtpBooks::stream(ostream &os, const string &instrument, bool isImage) {
    auto itr = entries.find(instrument);
    if (itr == entries.end()) {
        os << "no book found for " << instrument;
    }

    if (isImage)
        itr->second.image_stream(os);
    else
        itr->second.book_stream(os);
}

bool CtpBooks::update(const MktDataPayload &payload) {
    string instrument{payload.get_data().InstrumentID};
    auto itr = entries.find(instrument);
    if (itr == entries.end()) return false;

    itr->second.update_image(payload.get_data());
    return true;
}
