#include "TradeSpi.h"
#include "helper/CtpVisualHelper.h"
#include "utils/log/Log.h"

using namespace std;

void TradeSpi::OnFrontConnected() {
    MIDAS_LOG_INFO("TradeSpi OnFrontConnected OK");
    data->tradeLogInTime = ntime();
    manager->request_login(true);
}

void TradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) {
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("TradeSpi request ID " << nRequestID << " OnRspUserLogin: " << *pRspUserLogin);

        // save session parameters to test if my own order
        data->frontId = pRspUserLogin->FrontID;
        data->sessionId = pRspUserLogin->SessionID;
        int iNextOrderRef = atoi(pRspUserLogin->MaxOrderRef);
        iNextOrderRef++;
        sprintf(data->orderRef, "%d", iNextOrderRef);
        sprintf(data->execOrderRef, "%d", 1);

        MIDAS_LOG_INFO("current trading day = " << (manager->traderApi->GetTradingDay()));
        ///投资者结算结果确认
        manager->request_confirm_settlement();

        if (data->state == CtpState::TradeLogging) {
            std::lock_guard<std::mutex> lk(data->ctpMutex);
            MIDAS_LOG_INFO("TradeSpi trade logged");
            data->state = CtpState::TradeLogged;
            data->ctpCv.notify_one();
        }
    }
}

void TradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("TradeSpi request ID " << nRequestID << " OnRspSettlementInfoConfirm "
                                              << *pSettlementInfoConfirm);
    }
}

void TradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    if (pInstrument) data->instrumentInfo.insert(make_pair(string(pInstrument->InstrumentID), *pInstrument));

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " query instrument success.");
        if (data->state == CtpState::TradeInit) {
            sleep(1);
            manager->query_trading_account();
        }
    } else {
        /**
         * no need to log error in this block, because if this is error msg,
         * then bIsLast must be true, then error logged in IsErrorRspInfo(pRspInfo) function
         */
    }
}

///请求查询交易所响应
void TradeSpi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                bool bIsLast) {
    if (pExchange) data->exchanges.insert(make_pair(string(pExchange->ExchangeID), *pExchange));

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " query exchange success.");
        if (data->state == CtpState::TradeInit) {
            sleep(1);
            manager->query_product("");
        }
    }
}

///请求查询产品响应
void TradeSpi::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                               bool bIsLast) {
    if (pProduct) data->products.insert(make_pair(string(pProduct->ProductID), *pProduct));

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " query product success.");
        if (data->state == CtpState::TradeInit) {
            sleep(1);
            manager->query_instrument("");
        }
    }
}

void TradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
    if (pTradingAccount) data->accounts.insert(make_pair(string(pTradingAccount->AccountID), *pTradingAccount));

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " query account success.");
        if (data->state == CtpState::TradeInit) {
            sleep(1);
            manager->query_position("");
        }
    }
}

void TradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pInvestorPosition) {
        data->positions.push_back(*pInvestorPosition);
        MIDAS_LOG_DEBUG("request ID " << nRequestID << " OnRspQryInvestorPosition " << *pInvestorPosition);
    }

    if (bIsLast && !IsErrorRspInfo(pRspInfo)) {
        MIDAS_LOG_INFO("request ID " << nRequestID << " query position success.");
        if (data->state == CtpState::TradeInit) {
            std::lock_guard<std::mutex> lk(data->ctpMutex);
            data->state = CtpState::TradeInitFinished;
            data->ctpCv.notify_one();
        }
        // 报单录入请求
        // request_insert_order();
        // 执行宣告录入请求
        // request_insert_execute_order();
    }
}

void TradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) {
    MIDAS_LOG_DEBUG("request ID " << nRequestID << " bIsLast " << bIsLast << " OnRspOrderInsert " << *pInputOrder);
    IsErrorRspInfo(pRspInfo);
}

void TradeSpi::OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast) {
    //如果执行宣告正确，则不会进入该回调
    MIDAS_LOG_DEBUG("request ID " << nRequestID << " bIsLast " << bIsLast << " OnRspExecOrderInsert "
                                  << *pInputExecOrder);
    IsErrorRspInfo(pRspInfo);
}

/**
 * 撤单响应。交易核心返回的含有错误信息的撤单响应
 */
void TradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) {
    MIDAS_LOG_DEBUG("request ID " << nRequestID << " bIsLast " << bIsLast << " withdraw OnRspOrderAction "
                                  << *pInputOrderAction);
    IsErrorRspInfo(pRspInfo);
}

/**
 * 交易所会再次验证撤单指令的合法性,如果交易所认为该指令不合法,交易核心通过此函数转发交易所给出的错误。
 * 如果交易所认为该指令合法,同样会返回对应报单的新状态(OnRtnOrder)
 */
void TradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) {
    MIDAS_LOG_ERROR("withdraw order error: " << *pOrderAction);
    IsErrorRspInfo(pRspInfo);
}

void TradeSpi::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInpuExectOrderAction,
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    //正确的撤单操作，不会进入该回调
    MIDAS_LOG_DEBUG("request ID " << nRequestID << " bIsLast " << bIsLast << " OnRspExecOrderAction "
                                  << *pInpuExectOrderAction);
    IsErrorRspInfo(pRspInfo);
}

///报单通知
void TradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    MIDAS_LOG_DEBUG("OnRtnOrder " << *pOrder);
    if (IsMyOrder(pOrder)) {
        if (IsTradingOrder(pOrder))
            manager->ReqOrderAction(pOrder);
        else if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)
            MIDAS_LOG_DEBUG("cancel order success");
    }
}

//执行宣告通知
void TradeSpi::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {
    MIDAS_LOG_DEBUG("OnRtnExecOrder" << *pExecOrder);
    if (IsMyExecOrder(pExecOrder)) {
        if (IsTradingExecOrder(pExecOrder))
            manager->ReqExecOrderAction(pExecOrder);
        else if (pExecOrder->ExecResult == THOST_FTDC_OER_Canceled)
            MIDAS_LOG_DEBUG("执行宣告撤单成功");
    }
}

/**
 * 建议客户端以成交通知为准判断报单是否成交,以及成交数量和价格
 * 若以报单回报(状态为“部分成交或全不成交”)为准,由于报单回报与成交回报之间存在理论上的时间差,有可能导致平仓不成功
 * @param pTrade
 */
void TradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade) { MIDAS_LOG_DEBUG("OnRtnTrade " << *pTrade); }

void TradeSpi::OnFrontDisconnected(int nReason) {
    data->tradeLogOutTime = ntime();
    MIDAS_LOG_ERROR("--->>> TradeSpi OnFrontDisconnected Reason = " << ctp_disconnect_reason(nReason));
}

void TradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) { IsErrorRspInfo(pRspInfo); }

bool TradeSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
        MIDAS_LOG_ERROR("TradeSpi ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << gbk2utf8(pRspInfo->ErrorMsg));
    return bResult;
}

bool TradeSpi::IsMyOrder(CThostFtdcOrderField *pOrder) {
    return ((pOrder->FrontID == data->frontId) && (pOrder->SessionID == data->sessionId) &&
            (strcmp(pOrder->OrderRef, data->orderRef) == 0));
}

bool TradeSpi::IsMyExecOrder(CThostFtdcExecOrderField *pExecOrder) {
    return ((pExecOrder->FrontID == data->frontId) && (pExecOrder->SessionID == data->sessionId) &&
            (strcmp(pExecOrder->ExecOrderRef, data->execOrderRef) == 0));
}

bool TradeSpi::IsTradingOrder(CThostFtdcOrderField *pOrder) {
    return ((pOrder->OrderStatus != THOST_FTDC_OST_PartTradedNotQueueing) &&
            (pOrder->OrderStatus != THOST_FTDC_OST_Canceled) && (pOrder->OrderStatus != THOST_FTDC_OST_AllTraded));
}

bool TradeSpi::IsTradingExecOrder(CThostFtdcExecOrderField *pExecOrder) {
    return (pExecOrder->ExecResult != THOST_FTDC_OER_Canceled);
}

TradeSpi::TradeSpi(shared_ptr<TradeManager> manager_, shared_ptr<CtpData> d) : manager(manager_), data(d) {}
