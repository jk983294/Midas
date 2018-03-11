#ifndef MIDAS_TRADE_MANAGER_H
#define MIDAS_TRADE_MANAGER_H

#include <ctp/ThostFtdcMdApi.h>
#include <ctp/ThostFtdcTraderApi.h>
#include <string>
#include "model/CtpData.h"
#include "model/CtpDef.h"

using namespace std;

class TradeManager {
public:
    CThostFtdcTraderApi* traderApi;
    shared_ptr<CtpData> data;
    int tradeRequestId{0};  // 请求编号

public:
    TradeManager(shared_ptr<CtpData> d);

    void register_trader_api(CThostFtdcTraderApi* a) { traderApi = a; }

    ///用户登录请求
    void request_login();
    ///投资者结算结果确认
    void request_confirm_settlement();
    ///报单录入请求
    void request_insert_order(string instrument);
    ///执行宣告录入请求
    void request_insert_execute_order(string instrument);
    ///报单操作请求
    void ReqOrderAction(CThostFtdcOrderField* pOrder);
    ///执行宣告操作请求
    void ReqExecOrderAction(CThostFtdcExecOrderField* pExecOrder);

    int request_buy_limit(string instrument, int volume, double limitPrice);
    int request_buy_market(string instrument, int volume);
    int request_buy_if_above(string instrument, int volume, double conditionPrice, double limitPrice);
    int request_buy_if_below(string instrument, int volume, double conditionPrice, double limitPrice);

    int request_sell_limit(string instrument, int volume, double limitPrice);
    int request_sell_market(string instrument, int volume);
    int request_sell_if_above(string instrument, int volume, double conditionPrice, double limitPrice);
    int request_sell_if_below(string instrument, int volume, double conditionPrice, double limitPrice);

    int request_withdraw_order(const CThostFtdcOrderField& order);

    void query_instrument(const string& name);

    void query_product(const string& name);

    void query_exchange(const string& name);
    ///请求查询资金账户
    void query_trading_account();
    ///请求查询投资者持仓
    void query_position(string instrument);

    /**
     * query sequence: query_exchange query_product query_instrument query_trading_account query_position
     * event driven, the this function only invoke query_exchange, the callback will invoke one by one
     * the last query action's callback will set CtpState to init finish
     * so that other invoke won't get the all chain queried
     */
    void init_ctp();

private:
    //    int request_open_position(string instrument, CtpOrderType type, bool isBuy, int volume,
    //                              double limitPrice, double conditionPrice,
    //                              TThostFtdcContingentConditionType conditionType);

    void fill_common_order(CThostFtdcInputOrderField& order, const string& instrument, CtpOrderType type, bool isBuy,
                           int volume);
    void fill_limit_order(CThostFtdcInputOrderField& order, double price);
    void fill_market_order(CThostFtdcInputOrderField& order);
    void fill_condition_order(CThostFtdcInputOrderField& order, TThostFtdcContingentConditionType conditionType,
                              double conditionPrice, double limitPrice);
};

#endif
