#ifndef MIDAS_TRADE_SPI_H
#define MIDAS_TRADE_SPI_H

#include <ctp/ThostFtdcTraderApi.h>
#include <string>
#include "TradeManager.h"

using namespace std;

class TradeSpi : public CThostFtdcTraderSpi {
public:
    shared_ptr<TradeManager> manager;
    shared_ptr<CtpData> data;

public:
    TradeSpi(shared_ptr<TradeManager> manager_, shared_ptr<CtpData> d);

public:
    virtual ~TradeSpi() {}

    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    virtual void OnFrontConnected();

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    virtual void OnFrontDisconnected(int nReason);

    ///登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast);

    ///投资者结算结果确认响应
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///请求查询合约响应
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo,
                                    int nRequestID, bool bIsLast);

    ///请求查询资金账户响应
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///请求查询投资者持仓响应
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///报单录入请求响应
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast);

    ///执行宣告录入请求响应
    virtual void OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast);

    ///报单操作请求响应
    virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast);

    virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

    ///执行宣告操作请求响应
    virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction,
                                      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///报单通知
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

    ///执行宣告通知
    virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder);

    ///成交通知
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

    ///请求查询交易所响应
    virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                  bool bIsLast);

    ///请求查询产品响应
    virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                                 bool bIsLast);

private:
    // 是否收到成功的响应
    bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
    // 是否我的报单回报
    bool IsMyOrder(CThostFtdcOrderField *pOrder);
    // 是否我的执行宣告回报
    bool IsMyExecOrder(CThostFtdcExecOrderField *pExecOrder);
    // 是否正在交易的报单
    bool IsTradingOrder(CThostFtdcOrderField *pOrder);
    // 是否未撤销的执行宣告
    bool IsTradingExecOrder(CThostFtdcExecOrderField *pExecOrder);
};

#endif
