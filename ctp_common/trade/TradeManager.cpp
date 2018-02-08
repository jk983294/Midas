#include "TradeManager.h"
#include "helper/CtpVisualHelper.h"

/**
 * 0 send success
 * -1 failed due to network issue
 * -2 failed due to unhandled request number exceed threshold
 * -3 failed due to request number per second exceed threshold
 * so -2 and -3 means you can sleep one second then request again
 */
bool IsFlowControl(int iResult) { return ((iResult == -2) || (iResult == -3)); }

void TradeManager::request_login(bool isTradeLogin) {
    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.UserID, data->investorId.c_str());
    strcpy(req.Password, data->password.c_str());
    int iResult = 0;
    if (isTradeLogin)
        iResult = traderApi->ReqUserLogin(&req, ++tradeRequestId);
    else
        iResult = mdApi->ReqUserLogin(&req, ++marketRequestId);

    MIDAS_LOG_INFO("send user login request: " << ctp_result(iResult));
}

void TradeManager::request_confirm_settlement() {
    CThostFtdcSettlementInfoConfirmField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    int iResult = traderApi->ReqSettlementInfoConfirm(&req, ++tradeRequestId);
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
        int iResult = traderApi->ReqQryTradingAccount(&req, ++tradeRequestId);
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
        int iResult = traderApi->ReqQryInvestorPosition(&req, ++tradeRequestId);
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

    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
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

    int iResult = traderApi->ReqExecOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_DEBUG("request execute order insert: " << iResult << ", " << ctp_result(iResult));
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

    int iResult = traderApi->ReqOrderAction(&req, ++tradeRequestId);
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

    int iResult = traderApi->ReqExecOrderAction(&req, ++tradeRequestId);
    MIDAS_LOG_DEBUG("request execute order action: " << iResult << ", " << ctp_result(iResult));
    EXECORDER_ACTION_SENT = true;
}

int TradeManager::request_buy_limit(string instrument, int volume, double price) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Limit, BuyFlag, volume);
    fill_limit_order(req, price);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("buy limit order [[ volume: " << volume << " price:" << price << "]] result: " << iResult);
    return iResult;
}

int TradeManager::request_buy_market(string instrument, int volume) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Market, BuyFlag, volume);
    fill_market_order(req);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("buy market order [[ volume: " << volume << "]] result: " << iResult);
    return iResult;
}

int TradeManager::request_buy_if_above(string instrument, int volume, double conditionPrice, double limitPrice) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Condition, BuyFlag, volume);
    fill_condition_order(req, THOST_FTDC_CC_LastPriceGreaterThanStopPrice, conditionPrice, limitPrice);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("buy if above order [[ volume: " << volume << " price:" << conditionPrice
                                                    << " limitPrice: " << limitPrice << " ]] result: " << iResult);
    return iResult;
}

int TradeManager::request_buy_if_below(string instrument, int volume, double conditionPrice, double limitPrice) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Condition, BuyFlag, volume);
    fill_condition_order(req, THOST_FTDC_CC_LastPriceLesserThanStopPrice, conditionPrice, limitPrice);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("buy if above order [[ volume: " << volume << " price:" << conditionPrice
                                                    << " limitPrice: " << limitPrice << " ]] result: " << iResult);
    return iResult;
}

int TradeManager::request_sell_limit(string instrument, int volume, double price) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Limit, SellFlag, volume);
    fill_limit_order(req, price);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("sell limit order [[ volume: " << volume << " price:" << price << "]] result: " << iResult);
    return iResult;
}

int TradeManager::request_sell_market(string instrument, int volume) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Market, SellFlag, volume);
    fill_market_order(req);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("sell market order [[ volume: " << volume << "]] result: " << iResult);
    return iResult;
}

int TradeManager::request_sell_if_above(string instrument, int volume, double conditionPrice, double limitPrice) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Condition, SellFlag, volume);
    fill_condition_order(req, THOST_FTDC_CC_LastPriceGreaterThanStopPrice, conditionPrice, limitPrice);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("sell if above order [[ volume: " << volume << " price:" << conditionPrice
                                                     << " limitPrice: " << limitPrice << " ]] result: " << iResult);
    return iResult;
}

int TradeManager::request_sell_if_below(string instrument, int volume, double conditionPrice, double limitPrice) {
    CThostFtdcInputOrderField req{0};
    fill_common_order(req, instrument, CtpOrderType::Condition, SellFlag, volume);
    fill_condition_order(req, THOST_FTDC_CC_LastPriceLesserThanStopPrice, conditionPrice, limitPrice);
    int iResult = traderApi->ReqOrderInsert(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("sell if above order [[ volume: " << volume << " price:" << conditionPrice
                                                     << " limitPrice: " << limitPrice << " ]] result: " << iResult);
    return iResult;
}

int TradeManager::request_withdraw_order(const CThostFtdcOrderField &order) {
    CThostFtdcInputOrderActionField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, order.BrokerID);
    strcpy(req.InvestorID, order.InvestorID);
    strcpy(req.OrderRef, order.OrderRef);
    req.FrontID = data->frontId;
    req.SessionID = data->sessionId;
    req.ActionFlag = THOST_FTDC_AF_Delete;
    strcpy(req.InstrumentID, order.InstrumentID);

    int iResult = traderApi->ReqOrderAction(&req, ++tradeRequestId);
    MIDAS_LOG_INFO("request withdraw order: " << iResult << ", " << ctp_result(iResult));
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
        int iResult = traderApi->ReqQryInstrument(&req, ++tradeRequestId);
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
        int iResult = traderApi->ReqQryProduct(&req, ++tradeRequestId);
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
        int iResult = traderApi->ReqQryExchange(&req, ++tradeRequestId);
        if (!IsFlowControl(iResult)) {
            break;
        } else {
            MIDAS_LOG_ERROR("query exchange: " << iResult << ", under flow control");
            sleep(1);
        }
    }
}

TradeManager::TradeManager(shared_ptr<CtpData> d) : data(d) {}

void TradeManager::init_ctp() {
    MIDAS_LOG_INFO("trade manager wait for trade logged...");
    std::unique_lock<std::mutex> lk(data->ctpMutex);
    data->ctpCv.wait(lk, [this] { return data->state == CtpState::TradeLogged; });

    MIDAS_LOG_INFO("trade manager start to init ctp...");
    data->state = CtpState::TradeInit;
    query_exchange("");
}

void TradeManager::fill_common_order(CThostFtdcInputOrderField &req, const string &instrument, CtpOrderType type,
                                     bool isBuy, int volume) {
    strcpy(req.BrokerID, data->brokerId.c_str());
    strcpy(req.InvestorID, data->investorId.c_str());
    strcpy(req.InstrumentID, instrument.c_str());
    strcpy(req.OrderRef, data->orderRef);
    req.Direction = (isBuy ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell);
    req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    req.VolumeTotalOriginal = volume;
    req.VolumeCondition = THOST_FTDC_VC_AV;               // 任何数量
    req.MinVolume = 1;                                    // 最小成交量
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;  //强平原因: 非强平
    req.IsAutoSuspend = 0;                                //自动挂起标志: 否
    req.UserForceClose = 0;                               //用户强评标志: 否
}

void TradeManager::fill_limit_order(CThostFtdcInputOrderField &order, double limitPrice) {
    order.LimitPrice = limitPrice;
    order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    order.TimeCondition = THOST_FTDC_TC_GFD;                // 当日有效
    order.ContingentCondition = THOST_FTDC_CC_Immediately;  //触发条件: 立即
}
void TradeManager::fill_market_order(CThostFtdcInputOrderField &req) {
    req.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
    req.LimitPrice = 0;                                   // place 0 here for market order
    req.TimeCondition = THOST_FTDC_TC_IOC;                // 立即成交否则撤销
    req.ContingentCondition = THOST_FTDC_CC_Immediately;  //触发条件: 立即
}
void TradeManager::fill_condition_order(CThostFtdcInputOrderField &req, TThostFtdcContingentConditionType conditionType,
                                        double conditionPrice, double limitPrice) {
    req.ContingentCondition = conditionType;
    req.StopPrice = conditionPrice;
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    req.LimitPrice = limitPrice;
    req.TimeCondition = THOST_FTDC_TC_GFD;
}

void TradeManager::subscribe_all_instruments() {
    vector<string> instruments;
    for (auto itr = data->instrumentInfo.begin(); itr != data->instrumentInfo.end(); ++itr) {
        instruments.push_back(itr->first);
    }
    subscribe_market_data(instruments);
}
void TradeManager::subscribe_market_data(const vector<string> &instruments) {
    int instrumentCount = (int)instruments.size();
    if (instrumentCount <= 0) return;

    char **ppInstrumentID = new char *[instrumentCount];
    for (int i = 0; i < instrumentCount; ++i) {
        ppInstrumentID[i] = new char[instruments[i].size() + 1];
        strcpy(ppInstrumentID[i], instruments[i].c_str());
        ppInstrumentID[i][instruments[i].size()] = '\0';
    }

    int iResult = mdApi->SubscribeMarketData(ppInstrumentID, instrumentCount);
    if (iResult == 0) {
        MIDAS_LOG_INFO("send subscribe market data request OK! " << instrumentCount << " instruments get subscribed.");
    } else {
        MIDAS_LOG_ERROR("send subscribe market data request failed!")
    }

    for (int i = 0; i < instrumentCount; ++i) {
        delete[] ppInstrumentID[i];
    }
    delete[] ppInstrumentID;
}
