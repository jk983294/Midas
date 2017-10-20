#include "../helper/CtpVisualHelper.h"
#include "MdSpi.h"

using namespace std;

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    MIDAS_LOG_DEBUG("CtpMdSpi OnRspError requestId: " << nRequestID << " isLast:" << bIsLast);
    IsErrorRspInfo(pRspInfo);
}

void CtpMdSpi::OnFrontDisconnected(int nReason) {
    MIDAS_LOG_INFO("--->>> CtpMdSpi OnFrontDisconnected Reason = " << nReason);
}

void CtpMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    MIDAS_LOG_ERROR("--->>> should not see this since decommissioned, OnHeartBeatWarning nTimerLapse = " << nTimeLapse);
}

void CtpMdSpi::OnFrontConnected() {
    MIDAS_LOG_INFO("CtpMdSpi OnFrontConnected OK");
    manager->request_login(false);
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) {
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("CtpMdSpi request ID " << nRequestID << " OnRspUserLogin: " << *pRspUserLogin);
    }
}

void CtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspSubMarketData failed on " << *pSpecificInstrument);
        return;
    }
    if (pSpecificInstrument) {
        MIDAS_LOG_DEBUG("request ID " << nRequestID << " OnRspSubMarketData success on " << *pSpecificInstrument)
    }
}

void CtpMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                   CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspSubForQuoteRsp failed on " << *pSpecificInstrument);
        return;
    }
    if (pSpecificInstrument) {
        MIDAS_LOG_DEBUG("request ID " << nRequestID << " OnRspSubForQuoteRsp success on " << *pSpecificInstrument)
    }
}

void CtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspUnSubMarketData failed on " << *pSpecificInstrument);
        return;
    }
    if (pSpecificInstrument) {
        MIDAS_LOG_DEBUG("request ID " << nRequestID << " OnRspUnSubMarketData success on " << *pSpecificInstrument)
    }
}

void CtpMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                     CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspUnSubForQuoteRsp failed on " << *pSpecificInstrument);
        return;
    }
    if (pSpecificInstrument) {
        MIDAS_LOG_DEBUG("request ID " << nRequestID << " OnRspUnSubForQuoteRsp success on " << *pSpecificInstrument)
    }
}

void CtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    //    MIDAS_LOG_DEBUG("OnRtnDepthMarketData " << *pDepthMarketData);
    dataCallback(*pDepthMarketData, 0, 1);
}

void CtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {
    MIDAS_LOG_DEBUG("OnRtnForQuoteRsp " << *pForQuoteRsp);
}

bool CtpMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult) cerr << "ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << gbk2utf8(pRspInfo->ErrorMsg) << endl;
    return bResult;
}
