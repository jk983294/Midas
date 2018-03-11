#include <midas/MidasConfig.h>
#include "MdSpi.h"
#include "helper/CtpVisualHelper.h"

using namespace std;

CtpMdSpi::CtpMdSpi(CThostFtdcMdApi *mdApi_) : mdApi(mdApi_) {
    const string root{"ctp.session"};
    brokerId = get_cfg_value<string>(root, "brokerId");
    investorId = get_cfg_value<string>(root, "investorId");
    password = get_cfg_value<string>(root, "password");
    if (brokerId.empty()) {
        MIDAS_LOG_ERROR("empty brokerId!");
    }
    if (investorId.empty()) {
        MIDAS_LOG_ERROR("empty investorId!");
    }
    if (password.empty()) {
        MIDAS_LOG_ERROR("empty password!");
    }
    MIDAS_LOG_INFO("using config for market SPI { "
                   << "brokerId: " << brokerId << ", "
                   << "investorId: " << investorId << " }");
}

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) { IsErrorRspInfo(pRspInfo); }

void CtpMdSpi::OnFrontDisconnected(int nReason) {
    mdLogOutTime = ntime();
    MIDAS_LOG_ERROR("--->>> CtpMdSpi OnFrontDisconnected Reason = " << ctp_disconnect_reason(nReason));
}

void CtpMdSpi::OnFrontConnected() {
    mdLogInTime = ntime();
    MIDAS_LOG_INFO("CtpMdSpi OnFrontConnected OK");
    request_login();
}

void CtpMdSpi::request_login() {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, brokerId.c_str());
    strcpy(req.UserID, investorId.c_str());
    strcpy(req.Password, password.c_str());
    int iResult = 0;
    mdApi->ReqUserLogin(&req, ++marketRequestId);
    MIDAS_LOG_INFO("submit user login request: " << ctp_result(iResult));
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) {
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("CtpMdSpi request ID " << nRequestID << " OnRspUserLogin: " << *pRspUserLogin);
        // in case ctp reconnect event, subscribe again
        subscribe_all_instruments();
    }
}

void CtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspSubMarketData failed on " << *pSpecificInstrument);
        return;
    }
    if (bIsLast && pSpecificInstrument) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " OnRspSubMarketData success on " << *pSpecificInstrument)
    }
}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    dataCallback(*pDepthMarketData, ntime(), 1);
}

bool CtpMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        MIDAS_LOG_ERROR("MdSpi ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << gbk2utf8(pRspInfo->ErrorMsg));
    return bResult;
}

void CtpMdSpi::subscribe_all_instruments(const map<string, std::shared_ptr<CThostFtdcInstrumentField>> &map) {
    std::lock_guard<mutex> g(instrumentMutex);
    instruments.clear();
    for (auto itr = map.begin(); itr != map.end(); ++itr) {
        instruments.insert(itr->first);
    }
    std::vector<string> symbols(instruments.begin(), instruments.end());
    subscribe_market_data(symbols);
}

void CtpMdSpi::subscribe_all_instruments() {
    std::lock_guard<mutex> g(instrumentMutex);
    std::vector<string> symbols(instruments.begin(), instruments.end());
    subscribe_market_data(symbols);
}

void CtpMdSpi::subscribe(string symbol) {
    std::lock_guard<mutex> g(instrumentMutex);
    instruments.insert(symbol);
    vector<string> symbols{symbol};
    subscribe_market_data(symbols);
}

void CtpMdSpi::subscribe_market_data(const vector<string> &toSubscribe) {
    int instrumentCount = (int)toSubscribe.size();
    if (instrumentCount <= 0) return;

    if (instrumentCount <= 0) return;

    char **ppInstrumentID = new char *[instrumentCount];
    for (int i = 0; i < instrumentCount; ++i) {
        ppInstrumentID[i] = new char[toSubscribe[i].size() + 1];
        strcpy(ppInstrumentID[i], toSubscribe[i].c_str());
        ppInstrumentID[i][toSubscribe[i].size()] = '\0';
    }

    int iResult = mdApi->SubscribeMarketData(ppInstrumentID, instrumentCount);
    if (iResult == 0) {
        MIDAS_LOG_INFO("submit subscribe market data request OK! " << instrumentCount
                                                                   << " toSubscribe get subscribed.");
    } else {
        MIDAS_LOG_ERROR("submit subscribe market data request failed!")
    }

    for (int i = 0; i < instrumentCount; ++i) {
        delete[] ppInstrumentID[i];
    }
    delete[] ppInstrumentID;
}
