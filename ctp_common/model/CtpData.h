#ifndef MIDAS_CTP_DATA_H
#define MIDAS_CTP_DATA_H

#include <ctp/ThostFtdcUserApiDataType.h>
#include <utils/convert/StringHelper.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "CtpInstrument.h"
#include "tbb/concurrent_hash_map.h"
#include "trade/TradeStatusManager.h"

using namespace std;
using namespace midas;

enum CtpState { TradeLogging, TradeLogged, TradeInit, TradeInitFinished, MarketInit, Running, Closing };

class CtpData {
public:
    typedef tbb::concurrent_hash_map<string, string, MidasStringHashCompare> TMapSS;

    uint64_t mdLogInTime, mdLogOutTime, tradeLogInTime, tradeLogOutTime;
    std::atomic<CtpState> state{CtpState::TradeLogging};
    std::mutex ctpMutex;
    std::condition_variable ctpCv;

    string brokerId;    // 经纪公司代码
    string investorId;  // 投资者代码
    string password;    // 用户密码
    string tradeFront;
    string marketFront;
    string dataDirectory, tradeFlowPath, marketFlowPath;

    // 会话参数
    TThostFtdcFrontIDType frontId;        //前置编号
    TThostFtdcSessionIDType sessionId;    //会话编号
    TThostFtdcOrderRefType orderRef;      //报单引用
    TThostFtdcOrderRefType execOrderRef;  //执行宣告引用

    map<string, CThostFtdcInstrumentField> instrumentInfo;
    map<string, CThostFtdcExchangeField> exchanges;
    map<string, CThostFtdcProductField> products;
    map<string, CThostFtdcTradingAccountField> accounts;
    vector<CThostFtdcInvestorPositionField> positions;

    map<string, std::shared_ptr<CtpInstrument>> instruments;
    TradeStatusManager tradeStatusManager;

    TMapSS user2asyncData;

public:
    void init_all_instruments();

    void stream(ostream& os, const string& instrument, bool isImage);

    bool update(const MktDataPayload& payload);
};

#endif
