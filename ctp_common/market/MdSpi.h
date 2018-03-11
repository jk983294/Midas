#ifndef MIDAS_MD_SPI_H
#define MIDAS_MD_SPI_H

#include <ctp/ThostFtdcMdApi.h>
#include <set>
#include <string>
#include "trade/TradeManager.h"

using namespace std;

class CtpMdSpi : public CThostFtdcMdSpi {
public:
    typedef std::shared_ptr<CtpMdSpi> SharedPtr;
    typedef std::function<size_t(const CThostFtdcDepthMarketDataField &, uint64_t /*rcvt*/, int64_t /*id*/)> TCallback;

    TCallback dataCallback;
    CThostFtdcMdApi *mdApi{nullptr};
    uint64_t mdLogInTime{0}, mdLogOutTime{0};
    int marketRequestId{0};
    set<string> instruments;
    std::mutex instrumentMutex;
    string brokerId;    // 经纪公司代码
    string investorId;  // 投资者代码
    string password;    // 用户密码

public:
    CtpMdSpi(CThostFtdcMdApi *mdApi_);

    virtual ~CtpMdSpi() {}
    void register_data_callback(const TCallback &cb) { dataCallback = cb; }

    void subscribe_all_instruments(const map<string, std::shared_ptr<CThostFtdcInstrumentField>> &map);
    void subscribe_all_instruments();
    void subscribe(string symbol);

public:
    ///错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    virtual void OnFrontConnected();

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    ///        0x1001 网络读失败
    ///        0x1002 网络写失败
    ///        0x2001 接收心跳超时
    ///        0x2002 发送心跳失败
    ///        0x2003 收到错误报文
    virtual void OnFrontDisconnected(int nReason);

    ///登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast);

    ///订阅行情应答
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///深度行情通知
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

private:
    bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);

    void request_login();

    void subscribe_market_data(const vector<string> &toSubscribe);
};

#endif
