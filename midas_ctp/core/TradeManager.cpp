#include "../helper/CtpVisualHelper.h"
#include "TradeManager.h"
#include "utils/log/Log.h"

/**
 * 0 send success
 * -1 failed due to network issue
 * -2 failed due to unhandled request number exceed threshold
 * -3 failed due to request number per second exceed threshold
 * so -2 and -3 means you can sleep one second then request again
 */
bool IsFlowControl(int iResult) { return ((iResult == -2) || (iResult == -3)); }

void TradeManager::request_login() {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.UserID, data->investorId.c_str());
    strcpy(req.Password, data->password.c_str());
    int iResult = api->ReqUserLogin(&req, ++requestId);
    MIDAS_LOG_INFO("send user login request: " << ctp_result(iResult));
}

void TradeManager::request_confirm_settlement() {
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    int iResult = api->ReqSettlementInfoConfirm(&req, ++requestId);
    if (iResult == 0) {
        MIDAS_LOG_INFO("settlement info confirm OK");
    } else {
        MIDAS_LOG_ERROR("settlement info confirm failed!")
    }
}

void TradeManager::query_trading_account() {
    CThostFtdcQryTradingAccountField req{0};
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    while (true) {
        int iResult = api->ReqQryTradingAccount(&req, ++requestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query cash account: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

void TradeManager::query_position(string instrument) {
    CThostFtdcQryInvestorPositionField req{0};
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    while (true) {
        int iResult = api->ReqQryInvestorPosition(&req, ++requestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query investor position: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

void TradeManager::request_insert_order(string instrument) {
    CThostFtdcInputOrderField req{0};
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.OrderRef, data->orderRef);
    ///报单价格条件: 限价
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ///买卖方向:
    req.Direction = THOST_FTDC_D_Sell;
    ///组合开平标志: 开仓
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ///组合投机套保标志
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    ///价格
    req.LimitPrice = 60000;
    ///数量: 1
    req.VolumeTotalOriginal = 1;
    ///有效期类型: 当日有效
    req.TimeCondition = THOST_FTDC_TC_GFD;
    ///成交量类型: 任何数量
    req.VolumeCondition = THOST_FTDC_VC_AV;
    ///最小成交量: 1
    req.MinVolume = 1;
    ///触发条件: 立即
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    ///强平原因: 非强平
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ///自动挂起标志: 否
    req.IsAutoSuspend = 0;
    ///用户强评标志: 否
    req.UserForceClose = 0;

    int iResult = api->ReqOrderInsert(&req, ++requestId);
    MIDAS_LOG_DEBUG("request order insert: " << iResult << ", " << ctp_result(iResult));
}

//执行宣告录入请求
void TradeManager::request_insert_execute_order(string instrument) {
    CThostFtdcInputExecOrderField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.ExecOrderRef, data->execOrderRef);
    ///数量
    req.Volume = 1;
    ///开平标志
    req.OffsetFlag = THOST_FTDC_OF_Close;  //如果是上期所，需要填平今或平昨
    ///投机套保标志
    req.HedgeFlag = THOST_FTDC_HF_Speculation;
    ///执行类型
    req.ActionType = THOST_FTDC_ACTP_Exec;  //如果放弃执行则填THOST_FTDC_ACTP_Abandon
    ///保留头寸申请的持仓方向
    req.PosiDirection = THOST_FTDC_PD_Long;
    ///期权行权后是否保留期货头寸的标记
    req.ReservePositionFlag = THOST_FTDC_EOPF_UnReserve;  //这是中金所的填法，大商所郑商所填THOST_FTDC_EOPF_Reserve
    ///期权行权后生成的头寸是否自动平仓
    req.CloseFlag = THOST_FTDC_EOCF_AutoClose;  //这是中金所的填法，大商所郑商所填THOST_FTDC_EOCF_NotToClose

    int iResult = api->ReqExecOrderInsert(&req, ++requestId);
    MIDAS_LOG_DEBUG("request execute order insert: " << iResult << ", " << ctp_result(iResult));
}

//询价录入请求
void TradeManager::ReqForQuoteInsert(string instrument) {
    CThostFtdcInputForQuoteField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.ForQuoteRef, data->execOrderRef);

    int iResult = api->ReqForQuoteInsert(&req, ++requestId);
    MIDAS_LOG_DEBUG("request for quote insert: " << iResult << ", " << ctp_result(iResult));
}

//报价录入请求
void TradeManager::ReqQuoteInsert(string instrument) {
    CThostFtdcInputQuoteField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.QuoteRef, data->quoteRef);
    req.AskPrice = 60000;
    req.BidPrice = 60000 - 1.0;
    req.AskVolume = 1;
    req.BidVolume = 1;
    ///卖开平标志
    req.AskOffsetFlag = THOST_FTDC_OF_Open;
    ///买开平标志
    req.BidOffsetFlag = THOST_FTDC_OF_Open;
    ///卖投机套保标志
    req.AskHedgeFlag = THOST_FTDC_HF_Speculation;
    ///买投机套保标志
    req.BidHedgeFlag = THOST_FTDC_HF_Speculation;

    int iResult = api->ReqQuoteInsert(&req, ++requestId);
    MIDAS_LOG_DEBUG("request quote insert: " << iResult << ", " << ctp_result(iResult));
}

void TradeManager::ReqOrderAction(CThostFtdcOrderField *pOrder) {
    static bool ORDER_ACTION_SENT = false;  //是否发送了报单
    if (ORDER_ACTION_SENT) return;

    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, pOrder->BrokerID);
    strcpy(req.InvestorID, pOrder->InvestorID);
    ///报单操作引用
    //	TThostFtdcOrderActionRefType	OrderActionRef;
    strcpy(req.OrderRef, pOrder->OrderRef);
    ///请求编号
    //	TThostFtdcRequestIDType	RequestID;
    req.FrontID = data->frontId;
    req.SessionID = data->sessionId;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///价格
    //	TThostFtdcPriceType	LimitPrice;
    ///数量变化
    //	TThostFtdcVolumeType	VolumeChange;
    ///用户代码
    //	TThostFtdcUserIDType	UserID;
    ///合约代码
    strcpy(req.InstrumentID, pOrder->InstrumentID);

    int iResult = api->ReqOrderAction(&req, ++requestId);
    MIDAS_LOG_DEBUG("request order action: " << iResult << ", " << ctp_result(iResult));
    ORDER_ACTION_SENT = true;
}

void TradeManager::ReqExecOrderAction(CThostFtdcExecOrderField *pExecOrder) {
    static bool EXECORDER_ACTION_SENT = false;  //是否发送了报单
    if (EXECORDER_ACTION_SENT) return;

    CThostFtdcInputExecOrderActionField req;
    memset(&req, 0, sizeof(req));

    ///经纪公司代码
    strcpy(req.BrokerID, pExecOrder->BrokerID);
    ///投资者代码
    strcpy(req.InvestorID, pExecOrder->InvestorID);
    ///执行宣告操作引用
    // TThostFtdcOrderActionRefType	ExecOrderActionRef;
    ///执行宣告引用
    strcpy(req.ExecOrderRef, pExecOrder->ExecOrderRef);
    ///请求编号
    // TThostFtdcRequestIDType	RequestID;
    ///前置编号
    req.FrontID = data->frontId;
    ///会话编号
    req.SessionID = data->sessionId;
    ///交易所代码
    // TThostFtdcExchangeIDType	ExchangeID;
    ///执行宣告操作编号
    // TThostFtdcExecOrderSysIDType	ExecOrderSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///用户代码
    // TThostFtdcUserIDType	UserID;
    ///合约代码
    strcpy(req.InstrumentID, pExecOrder->InstrumentID);

    int iResult = api->ReqExecOrderAction(&req, ++requestId);
    MIDAS_LOG_DEBUG("request execute order action: " << iResult << ", " << ctp_result(iResult));
    EXECORDER_ACTION_SENT = true;
}

void TradeManager::ReqQuoteAction(CThostFtdcQuoteField *pQuote) {
    static bool QUOTE_ACTION_SENT = false;  //是否发送了报单
    if (QUOTE_ACTION_SENT) return;

    CThostFtdcInputQuoteActionField req;
    memset(&req, 0, sizeof(req));
    ///经纪公司代码
    strcpy(req.BrokerID, pQuote->BrokerID);
    ///投资者代码
    strcpy(req.InvestorID, pQuote->InvestorID);
    ///报价操作引用
    // TThostFtdcOrderActionRefType	QuoteActionRef;
    ///报价引用
    strcpy(req.QuoteRef, pQuote->QuoteRef);
    ///请求编号
    // TThostFtdcRequestIDType	RequestID;
    ///前置编号
    req.FrontID = data->frontId;
    ///会话编号
    req.SessionID = data->sessionId;
    ///交易所代码
    // TThostFtdcExchangeIDType	ExchangeID;
    ///报价操作编号
    // TThostFtdcOrderSysIDType	QuoteSysID;
    ///操作标志
    req.ActionFlag = THOST_FTDC_AF_Delete;
    ///用户代码
    // TThostFtdcUserIDType	UserID;
    ///合约代码
    strcpy(req.InstrumentID, pQuote->InstrumentID);

    int iResult = api->ReqQuoteAction(&req, ++requestId);
    MIDAS_LOG_DEBUG("request quote action: " << iResult << ", " << ctp_result(iResult));
    QUOTE_ACTION_SENT = true;
}

int TradeManager::request_buy(string instrument, double limitPrice, int volume) {
    int iResult = request_open_position(instrument, limitPrice, volume, true);
    MIDAS_LOG_INFO("request buy order insert: " << iResult);
    return iResult;
}

int TradeManager::request_sell(string instrument, double limitPrice, int volume) {
    int iResult = request_open_position(instrument, limitPrice, volume, false);
    MIDAS_LOG_INFO("request sell order insert: " << iResult);
    return iResult;
}

/**
 * query instrument, like cu1712
 * @param name if empty name provided, then query all instruments
 */
void TradeManager::query_instrument(const string &name) {
    CThostFtdcQryInstrumentField req = {0};
    // memset(&req, 0, sizeof(req));
    strcpy(req.InstrumentID, name.c_str());
    while (true) {
        int iResult = api->ReqQryInstrument(&req, ++requestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query instrument: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

/**
 * query product, like cu
 * @param name if empty name provided, then query all products
 */
void TradeManager::query_product(const string &name) {
    CThostFtdcQryProductField req = {0};
    strcpy(req.ProductID, name.c_str());
    while (true) {
        int iResult = api->ReqQryProduct(&req, ++requestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query instrument: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

/**
 * query exchange, like SHFE
 * ExchangeID: CFFEX ExchangeName: 中国金融交易所 ExchangeProperty: 0
 * ExchangeID: CZCE ExchangeName: 郑州商品交易所 ExchangeProperty: 0
 * ExchangeID: DCE ExchangeName: 大连商品交易所 ExchangeProperty: 1
 * ExchangeID: INE ExchangeName: 上海国际能源交易中心股份有限公司 ExchangeProperty: 0
 * ExchangeID: SHFE ExchangeName: 上海期货交易所 ExchangeProperty: 0
 * @param name if empty name provided, then query all exchanges
 */
void TradeManager::query_exchange(const string &name) {
    CThostFtdcQryExchangeField req = {0};
    strcpy(req.ExchangeID, name.c_str());
    while (true) {
        int iResult = api->ReqQryExchange(&req, ++requestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query exchange: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

TradeManager::TradeManager(CThostFtdcTraderApi *a, CtpData *d) : api(a), data(d) {}

void TradeManager::init_ctp() {
    data->state = TradeInit;
    query_exchange("");
}

int TradeManager::request_open_position(string instrument, double limitPrice, int volume, bool isBuy) {
    CThostFtdcInputOrderField req{0};
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.OrderRef, data->orderRef);
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

    if (isBuy)
        req.Direction = THOST_FTDC_D_Buy;
    else
        req.Direction = THOST_FTDC_D_Sell;

    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    req.LimitPrice = limitPrice;
    req.VolumeTotalOriginal = volume;
    req.TimeCondition = THOST_FTDC_TC_GFD;                // 当日有效
    req.VolumeCondition = THOST_FTDC_VC_AV;               // 任何数量
    req.MinVolume = 1;                                    // 最小成交量
    req.ContingentCondition = THOST_FTDC_CC_Immediately;  //触发条件: 立即
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;  //强平原因: 非强平
    req.IsAutoSuspend = 0;                                //自动挂起标志: 否
    req.UserForceClose = 0;                               //用户强评标志: 否

    return api->ReqOrderInsert(&req, ++requestId);
}
