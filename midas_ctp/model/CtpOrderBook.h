#ifndef MIDAS_CTP_ORDER_BOOK_H
#define MIDAS_CTP_ORDER_BOOK_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <net/disruptor/Payload.h>
#include <iostream>
#include <map>
#include <string>

using namespace std;
using namespace midas;

typedef midas::PayloadObject<CThostFtdcDepthMarketDataField> MktDataPayload;

class CtpBookEntry {
public:
    string instrument;
    CThostFtdcDepthMarketDataField image;
    long updateCount;

public:
    CtpBookEntry(const string& instrument);

    void book_stream(ostream& os);

    void image_stream(ostream& os);

    void update_image(const CThostFtdcDepthMarketDataField& img);
};

class CtpBooks {
public:
    std::map<string, CtpBookEntry> entries;

public:
    void init_all_instruments(const map<string, CThostFtdcInstrumentField>& instruments);

    void stream(ostream& os, const string& instrument, bool isImage);

    bool update(const MktDataPayload& payload);
};

#endif
