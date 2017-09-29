#ifndef MIDAS_TRADE_MANAGER_H
#define MIDAS_TRADE_MANAGER_H

#include <ctp/ThostFtdcTraderApi.h>
#include <string>
#include "../model/CtpData.h"

using namespace std;

class TradeManager {
public:
    CThostFtdcTraderApi *api;
    CtpData *data;
    int requestId = 0;  // 请求编号

public:
    TradeManager(CThostFtdcTraderApi *a, CtpData *d);

    ///用户登录请求
    void request_login();
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
    void ReqOrderAction(CThostFtdcOrderField *pOrder);
    ///执行宣告操作请求
    void ReqExecOrderAction(CThostFtdcExecOrderField *pExecOrder);
    ///报价操作请求
    void ReqQuoteAction(CThostFtdcQuoteField *pQuote);

    void query_instrument(const string &name);

    void query_product(const string &name);
    ///请求查询资金账户
    void query_trading_account();
    ///请求查询投资者持仓
    void query_position(string instrument);
};

#endif
