#include "../helper/CtpVisualHelper.h"
#include "MdSpi.h"

using namespace std;

void CtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) { IsErrorRspInfo(pRspInfo); }

void CtpMdSpi::OnFrontDisconnected(int nReason) {
    data->mdLogOutTime = ntime();
    MIDAS_LOG_ERROR("--->>> CtpMdSpi OnFrontDisconnected Reason = " << ctp_disconnect_reason(nReason));
}

void CtpMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    MIDAS_LOG_ERROR("--->>> should not see this since decommissioned, OnHeartBeatWarning nTimerLapse = " << nTimeLapse);
}

void CtpMdSpi::OnFrontConnected() {
    data->mdLogInTime = ntime();
    MIDAS_LOG_INFO("CtpMdSpi OnFrontConnected OK");
    manager->request_login(false);
}

void CtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) {
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("CtpMdSpi request ID " << nRequestID << " OnRspUserLogin: " << *pRspUserLogin);
        // in case ctp reconnect event, subscribe again
        if (data->state == Running) {
            manager->subscribe_all_instruments();
        }
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

void CtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_ERROR("request ID " << nRequestID << " OnRspUnSubMarketData failed on " << *pSpecificInstrument);
        return;
    }
    if (pSpecificInstrument) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " OnRspUnSubMarketData success on " << *pSpecificInstrument)
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
