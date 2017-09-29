#include "MdSpi.h"
#include <iostream>

using namespace std;

void CMdSpi::init(CThostFtdcMdApi *mdApi_, string brokerId_, string investorId_, string password_,
                  vector<string> instruments) {
    mdApi = mdApi_;
    brokerId = brokerId_;
    investorId = investorId_;
    password = password_;
    iInstrumentID = (int)instruments.size();
    ppInstrumentID = new char *[iInstrumentID];
    for (int i = 0; i < iInstrumentID; ++i) {
        ppInstrumentID[i] = new char[instruments[i].size() + 1];
        strcpy(ppInstrumentID[i], instruments[i].c_str());
        ppInstrumentID[i][instruments[i].size()] = '\0';
    }
}

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    cout << "--->>> "
         << "OnRspError" << endl;
    IsErrorRspInfo(pRspInfo);
}

void CMdSpi::OnFrontDisconnected(int nReason) { cout << "--->>> OnFrontDisconnected Reason = " << nReason << endl; }

void CMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    cerr << "--->>> should not see this since decommissioned, OnHeartBeatWarning nTimerLapse = " << nTimeLapse << endl;
}

void CMdSpi::OnFrontConnected() {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, brokerId.c_str());
    strcpy(req.UserID, investorId.c_str());
    strcpy(req.Password, password.c_str());
    int iResult = mdApi->ReqUserLogin(&req, ++iRequestID);
    cout << "--->>> OnFrontConnected send user login request: " << ((iResult == 0) ? "success" : "fail") << endl;
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                            int nRequestID, bool bIsLast) {
    cout << "--->>> "
         << "OnRspUserLogin" << endl;
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        cout << "user login success: " << '\n'
             << "TradingDay: " << pRspUserLogin->TradingDay << '\n'
             << "LoginTime: " << pRspUserLogin->LoginTime << '\n'
             << "UserID: " << pRspUserLogin->UserID << '\n'
             << "SystemName: " << pRspUserLogin->SystemName << '\n'
             << "FrontID: " << pRspUserLogin->FrontID << '\n'
             << "SessionID: " << pRspUserLogin->SessionID << '\n'
             << "MaxOrderRef: " << pRspUserLogin->MaxOrderRef << '\n';

        ///获取当前交易日
        // cout << "--->>> current trading day = " << mdApi->GetTradingDay() << endl;
        // 请求订阅行情
        SubscribeMarketData();
        // 请求订阅询价,只能订阅郑商所的询价，其他交易所通过traderapi相应接口返回
        SubscribeForQuoteRsp();
    }
}

void CMdSpi::SubscribeMarketData() {
    int iResult = mdApi->SubscribeMarketData(ppInstrumentID, iInstrumentID);
    cout << "--->>> send subscribe market data request: " << ((iResult == 0) ? "success" : "fail") << endl;
}

void CMdSpi::SubscribeForQuoteRsp() {
    int iResult = mdApi->SubscribeForQuoteRsp(ppInstrumentID, iInstrumentID);
    cout << "--->>> send subscribe quote request: " << ((iResult == 0) ? "success" : "fail") << endl;
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    cout << "OnRspSubMarketData " << nRequestID << endl;
    if (!IsErrorRspInfo(pRspInfo)) {
        cout << "id " << pSpecificInstrument->InstrumentID << " ,;" << endl;
    }
}

void CMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    cout << "OnRspSubForQuoteRsp " << nRequestID << endl;
    if (!IsErrorRspInfo(pRspInfo)) {
        cout << "id " << pSpecificInstrument->InstrumentID << " ,;" << endl;
    }
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    cout << "OnRspUnSubMarketData" << endl;
}

void CMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    cout << "OnRspUnSubForQuoteRsp" << endl;
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    cout << "OnRtnDepthMarketData" << endl;
    cout << "e " << pDepthMarketData->ExchangeID << " ,"
         << "id " << pDepthMarketData->InstrumentID << " ,"
         << "tp " << pDepthMarketData->LastPrice << " ,"
         << "psp " << pDepthMarketData->PreSettlementPrice << " ,"
         << "ccp " << pDepthMarketData->PreClosePrice << " ,"
         << "poi " << pDepthMarketData->PreOpenInterest << " ,"
         << "op " << pDepthMarketData->OpenPrice << " ,"
         << "hp " << pDepthMarketData->HighestPrice << " ,"
         << "lp " << pDepthMarketData->LowestPrice << " ,"
         << "ts " << pDepthMarketData->Volume << " ,"
         << "to " << pDepthMarketData->Turnover << " ,"
         << "oi " << pDepthMarketData->OpenInterest << " ,"
         << "cp " << pDepthMarketData->ClosePrice << " ,"
         << "sp " << pDepthMarketData->SettlementPrice << " ,"
         << "ulp " << pDepthMarketData->UpperLimitPrice << " ,"
         << "llp " << pDepthMarketData->LowerLimitPrice << " ,"
         << "pd " << pDepthMarketData->PreDelta << " ,"
         << "cd " << pDepthMarketData->CurrDelta << " ,"
         << "ut " << pDepthMarketData->UpdateTime << " ,"
         << "um " << pDepthMarketData->UpdateMillisec << " ,"
         << "bbp1 " << pDepthMarketData->BidPrice1 << " ,"
         << "bbs1 " << pDepthMarketData->BidVolume1 << " ,"
         << "bbp2 " << pDepthMarketData->BidPrice2 << " ,"
         << "bbs2 " << pDepthMarketData->BidVolume2 << " ,"
         << "bbp3 " << pDepthMarketData->BidPrice3 << " ,"
         << "bbs3 " << pDepthMarketData->BidVolume3 << " ,"
         << "bbp4 " << pDepthMarketData->BidPrice4 << " ,"
         << "bbs4 " << pDepthMarketData->BidVolume4 << " ,"
         << "bbp5 " << pDepthMarketData->BidPrice5 << " ,"
         << "bbs5 " << pDepthMarketData->BidVolume5 << " ,"
         << "bap1 " << pDepthMarketData->AskPrice1 << " ,"
         << "bas1 " << pDepthMarketData->AskVolume1 << " ,"
         << "bap2 " << pDepthMarketData->AskPrice2 << " ,"
         << "bas2 " << pDepthMarketData->AskVolume2 << " ,"
         << "bap3 " << pDepthMarketData->AskPrice3 << " ,"
         << "bas3 " << pDepthMarketData->AskVolume3 << " ,"
         << "bap4 " << pDepthMarketData->AskPrice4 << " ,"
         << "bas4 " << pDepthMarketData->AskVolume4 << " ,"
         << "bap5 " << pDepthMarketData->AskPrice5 << " ,"
         << "bas5 " << pDepthMarketData->AskVolume5 << " ,"
         << "avgp " << pDepthMarketData->AveragePrice << " ,"
         << "ad " << pDepthMarketData->ActionDay << " ,"
         << ";\n";
}

void CMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) { cout << "OnRtnForQuoteRsp" << endl; }

bool CMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult) cerr << "--->>> ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << pRspInfo->ErrorMsg << endl;
    return bResult;
}
