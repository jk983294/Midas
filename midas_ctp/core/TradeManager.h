#ifndef MIDAS_TRADE_MANAGER_H
#define MIDAS_TRADE_MANAGER_H

#include <ctp/ThostFtdcMdApi.h>
#include <ctp/ThostFtdcTraderApi.h>
#include <string>
#include "../model/CtpData.h"

using namespace std;

class TradeManager {
public:
    CThostFtdcTraderApi* traderApi;
    CThostFtdcMdApi* mdApi;
    shared_ptr<CtpData> data;
    int tradeRequestId{0}, marketRequestId{0};  // 请求编号

public:
    TradeManager(shared_ptr<CtpData> d);

    void register_trader_api(CThostFtdcTraderApi* a) { traderApi = a; }
    void register_md_api(CThostFtdcMdApi* a) { mdApi = a; }

    ///用户登录请求
    void request_login(bool isTradeLogin);
    ///投资者结算结果确认
    void request_confirm_settlement();
    ///报单录入请求
    void request_insert_order(string instrument);
    ///执行宣告录入请求
    void request_insert_execute_order(string instrument);
    ///询价录入请求
    void ReqForQuoteInsert(string instrument);
    ///报价录入请求
    void ReqQuoteInsert(string instrument);
    ///报单操作请求
    void ReqOrderAction(CThostFtdcOrderField* pOrder);
    ///执行宣告操作请求
    void ReqExecOrderAction(CThostFtdcExecOrderField* pExecOrder);
    ///报价操作请求
    void ReqQuoteAction(CThostFtdcQuoteField* pQuote);

    int request_buy(string instrument, double limitPrice, int volume);

    int request_sell(string instrument, double limitPrice, int volume);

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

    void subscribe_market_data(const vector<string>& instruments);

    /**
     * 请求订阅询价,只能订阅郑商所的询价，其他交易所通过traderapi相应接口返回
     * @param instruments
     */
    void subscribe_quote_czce(const vector<string>& instruments);

private:
    int request_open_position(string instrument, double limitPrice, int volume, bool isBuy);
};

#endif
