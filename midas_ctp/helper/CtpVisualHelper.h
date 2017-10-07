#ifndef MIDAS_CTP_VISUAL_HELPER_H
#define MIDAS_CTP_VISUAL_HELPER_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include "CtpValueDescription.h"
#include "utils/VisualHelper.h"

using namespace std;
using namespace midas;

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcRspUserLoginField& field) {
    os << "TradingDay: " << field.TradingDay << " LoginTime: " << field.LoginTime << " BrokerID: " << field.BrokerID
       << " UserID: " << field.UserID << " SystemName: " << field.SystemName << " FrontID: " << field.FrontID
       << " SessionID: " << field.SessionID << " MaxOrderRef: " << field.MaxOrderRef << " SHFETime: " << field.SHFETime
       << " DCETime: " << field.DCETime << " CZCETime: " << field.CZCETime << " FFEXTime: " << field.FFEXTime;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcSettlementInfoConfirmField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID << " ConfirmDate: " << field.ConfirmDate
       << " ConfirmTime: " << field.ConfirmTime;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInstrumentField& field) {
    os << "InstrumentID: " << field.InstrumentID << " ExchangeID: " << field.ExchangeID
       << " ExchangeInstID: " << field.ExchangeInstID << " ProductID: " << field.ProductID
       << " ProductClass: " << ctp_product_class(field.ProductClass) << " DeliveryYear: " << field.DeliveryYear
       << " DeliveryMonth: " << field.DeliveryMonth << " MaxMarketOrderVolume: " << field.MaxMarketOrderVolume
       << " MinMarketOrderVolume: " << field.MinMarketOrderVolume
       << " MaxLimitOrderVolume: " << field.MaxLimitOrderVolume << " MinLimitOrderVolume: " << field.MinLimitOrderVolume
       << " VolumeMultiple: " << field.VolumeMultiple << " PriceTick: " << field.PriceTick
       << " CreateDate: " << field.CreateDate << " OpenDate: " << field.OpenDate << " ExpireDate: " << field.ExpireDate
       << " StartDelivDate: " << field.StartDelivDate << " EndDelivDate: " << field.EndDelivDate
       << " InstLifePhase: " << ctp_phase_type(field.InstLifePhase)
       << " IsTrading: " << get_bool_string(field.IsTrading)
       << " PositionType: " << ctp_position_type(field.PositionType)
       << " PositionDateType: " << ctp_position_date_type(field.PositionDateType)
       << " LongMarginRatio: " << field.LongMarginRatio << " ShortMarginRatio: " << field.ShortMarginRatio
       << " MaxMarginSideAlgorithm: " << ctp_mmsa(field.MaxMarginSideAlgorithm)
       << " UnderlyingInstrID: " << field.UnderlyingInstrID << " StrikePrice: " << field.StrikePrice
       << " OptionsType: " << ctp_options_type(field.OptionsType) << " UnderlyingMultiple: " << field.UnderlyingMultiple
       << " CombinationType: " << ctp_combination_type(field.CombinationType) << " MinBuyVolume: " << field.MinBuyVolume
       << " MinSellVolume: " << field.MinSellVolume << " InstrumentCode: " << field.InstrumentCode;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcTradingAccountField& field) {
    os << "BrokerID: " << field.BrokerID << " AccountID: " << field.AccountID << " PreMortgage: " << field.PreMortgage
       << " PreCredit: " << field.PreCredit << " PreDeposit: " << field.PreDeposit
       << " PreBalance: " << field.PreBalance << " PreMargin: " << field.PreMargin
       << " InterestBase: " << field.InterestBase << " Interest: " << field.Interest << " Deposit: " << field.Deposit
       << " Withdraw: " << field.Withdraw << " FrozenMargin: " << field.FrozenMargin
       << " FrozenCash: " << field.FrozenCash << " FrozenCommission: " << field.FrozenCommission
       << " CurrMargin: " << field.CurrMargin << " CashIn: " << field.CashIn << " Commission: " << field.Commission
       << " CloseProfit: " << field.CloseProfit << " PositionProfit: " << field.PositionProfit
       << " Balance: " << field.Balance << " Available: " << field.Available
       << " WithdrawQuota: " << field.WithdrawQuota << " Reserve: " << field.Reserve
       << " TradingDay: " << field.TradingDay << " SettlementID: " << field.SettlementID << " Credit: " << field.Credit
       << " Mortgage: " << field.Mortgage << " ExchangeMargin: " << field.ExchangeMargin
       << " DeliveryMargin: " << field.DeliveryMargin << " ExchangeDeliveryMargin: " << field.ExchangeDeliveryMargin
       << " ReserveBalance: " << field.ReserveBalance << " CurrencyID: " << field.CurrencyID
       << " PreFundMortgageIn: " << field.PreFundMortgageIn << " PreFundMortgageOut: " << field.PreFundMortgageOut
       << " FundMortgageIn: " << field.FundMortgageIn << " FundMortgageOut: " << field.FundMortgageOut
       << " FundMortgageAvailable: " << field.FundMortgageAvailable << " MortgageableFund: " << field.MortgageableFund
       << " SpecProductMargin: " << field.SpecProductMargin
       << " SpecProductFrozenMargin: " << field.SpecProductFrozenMargin
       << " SpecProductCommission: " << field.SpecProductCommission
       << " SpecProductFrozenCommission: " << field.SpecProductFrozenCommission
       << " SpecProductPositionProfit: " << field.SpecProductPositionProfit
       << " SpecProductCloseProfit: " << field.SpecProductCloseProfit
       << " SpecProductPositionProfitByAlg: " << field.SpecProductPositionProfitByAlg
       << " SpecProductExchangeMargin: " << field.SpecProductExchangeMargin
       << " BizType: " << ctp_business_type(field.BizType);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInvestorPositionField& field) {
    os << "InstrumentID: " << field.InstrumentID << " BrokerID: " << field.BrokerID
       << " InvestorID: " << field.InvestorID << " PosiDirection: " << ctp_position_direction(field.PosiDirection)
       << " HedgeFlag: " << field.HedgeFlag << " PositionDate: " << field.PositionDate
       << " YdPosition: " << field.YdPosition << " Position: " << field.Position
       << " PositionCost: " << field.PositionCost << " ShortFrozen: " << field.ShortFrozen
       << " LongFrozen: " << field.LongFrozen << " ShortFrozenAmount: " << field.ShortFrozenAmount
       << " OpenAmount: " << field.OpenAmount << " CloseAmount: " << field.CloseAmount
       << " PositionCost: " << field.PositionCost << " PreMargin: " << field.PreMargin
       << " UseMargin: " << field.UseMargin << " FrozenMargin: " << field.FrozenMargin
       << " FrozenCash: " << field.FrozenCash << " FrozenCommission: " << field.FrozenCommission
       << " CashIn: " << field.CashIn << " Commission: " << field.Commission << " CloseProfit: " << field.CloseProfit
       << " PositionProfit: " << field.PositionProfit << " PreSettlementPrice: " << field.PreSettlementPrice
       << " SettlementPrice: " << field.SettlementPrice << " TradingDay: " << field.TradingDay
       << " SettlementID: " << field.SettlementID << " OpenCost: " << field.OpenCost
       << " ExchangeMargin: " << field.ExchangeMargin << " CombPosition: " << field.CombPosition
       << " CombLongFrozen: " << field.CombLongFrozen << " CombShortFrozen: " << field.CombShortFrozen
       << " CloseProfitByDate: " << field.CloseProfitByDate << " CloseProfitByTrade: " << field.CloseProfitByTrade
       << " TodayPosition: " << field.TodayPosition << " MarginRateByMoney: " << field.MarginRateByMoney
       << " MarginRateByVolume: " << field.MarginRateByVolume << " StrikeFrozen: " << field.StrikeFrozen
       << " StrikeFrozenAmount: " << field.StrikeFrozenAmount << " AbandonFrozen: " << field.AbandonFrozen
       << " ExchangeID: " << field.ExchangeID << " YdStrikeFrozen: " << field.YdStrikeFrozen;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputOrderField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " OrderRef: " << field.OrderRef << " UserID: " << field.UserID
       << " OrderPriceType: " << field.OrderPriceType << " Direction: " << ctp_direction(field.Direction)
       << " CombOffsetFlag: " << field.CombOffsetFlag << " CombHedgeFlag: " << field.CombHedgeFlag
       << " LimitPrice: " << field.LimitPrice << " VolumeTotalOriginal: " << field.VolumeTotalOriginal
       << " TimeCondition: " << ctp_time_condition(field.TimeCondition) << " GTDDate: " << field.GTDDate
       << " VolumeCondition: " << ctp_volume_condition(field.VolumeCondition) << " MinVolume: " << field.MinVolume
       << " ContingentCondition: " << ctp_contingent_condition(field.ContingentCondition)
       << " StopPrice: " << field.StopPrice << " ForceCloseReason: " << ctp_force_close_reason(field.ForceCloseReason)
       << " IsAutoSuspend: " << field.IsAutoSuspend << " BusinessUnit: " << field.BusinessUnit
       << " RequestID: " << field.RequestID << " UserForceClose: " << field.UserForceClose
       << " IsSwapOrder: " << field.IsSwapOrder << " ExchangeID: " << field.ExchangeID
       << " InvestUnitID: " << field.InvestUnitID << " AccountID: " << field.AccountID
       << " CurrencyID: " << field.CurrencyID << " ClientID: " << field.ClientID << " IPAddress: " << field.IPAddress
       << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputExecOrderField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " ExecOrderRef: " << field.ExecOrderRef
       << " UserID: " << field.UserID << " Volume: " << field.Volume << " RequestID: " << field.RequestID
       << " BusinessUnit: " << field.BusinessUnit << " OffsetFlag: " << field.OffsetFlag
       << " HedgeFlag: " << field.HedgeFlag << " ActionType: " << field.ActionType
       << " PosiDirection: " << ctp_position_direction(field.PosiDirection)
       << " ReservePositionFlag: " << field.ReservePositionFlag << " CloseFlag: " << field.CloseFlag
       << " ExchangeID: " << field.ExchangeID << " InvestUnitID: " << field.InvestUnitID
       << " AccountID: " << field.AccountID << " CurrencyID: " << field.CurrencyID << " ClientID: " << field.ClientID
       << " IPAddress: " << field.IPAddress << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputForQuoteField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " ForQuoteRef: " << field.ForQuoteRef
       << " UserID: " << field.UserID << " ExchangeID: " << field.ExchangeID << " InvestUnitID: " << field.InvestUnitID
       << " IPAddress: " << field.IPAddress << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputQuoteField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " QuoteRef: " << field.QuoteRef << " UserID: " << field.UserID
       << " AskPrice: " << field.AskPrice << " BidPrice: " << field.BidPrice << " AskVolume: " << field.AskVolume
       << " BidVolume: " << field.BidVolume << " RequestID: " << field.RequestID
       << " BidOrderRef: " << field.BidOrderRef << " AskOffsetFlag: " << field.AskOffsetFlag
       << " BusinessUnit: " << field.BusinessUnit << " AskHedgeFlag: " << field.AskHedgeFlag
       << " BidHedgeFlag: " << field.BidHedgeFlag << " AskOrderRef: " << field.AskOrderRef
       << " BidOrderRef: " << field.BidOrderRef << " ForQuoteSysID: " << field.ForQuoteSysID
       << " ExchangeID: " << field.ExchangeID << " InvestUnitID: " << field.InvestUnitID
       << " ClientID: " << field.ClientID << " IPAddress: " << field.IPAddress << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputOrderActionField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " OrderActionRef: " << field.OrderActionRef << " OrderRef: " << field.OrderRef
       << " RequestID: " << field.RequestID << " FrontID: " << field.FrontID << " SessionID: " << field.SessionID
       << " ExchangeID: " << field.ExchangeID << " OrderSysID: " << field.OrderSysID
       << " ActionFlag: " << field.ActionFlag << " LimitPrice: " << field.LimitPrice
       << " VolumeChange: " << field.VolumeChange << " UserID: " << field.UserID
       << " InstrumentID: " << field.InstrumentID << " InvestUnitID: " << field.InvestUnitID
       << " IPAddress: " << field.IPAddress << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputExecOrderActionField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " ExecOrderActionRef: " << field.ExecOrderActionRef << " ExecOrderRef: " << field.ExecOrderRef
       << " RequestID: " << field.RequestID << " FrontID: " << field.FrontID << " SessionID: " << field.SessionID
       << " ExchangeID: " << field.ExchangeID << " ExecOrderSysID: " << field.ExecOrderSysID
       << " ActionFlag: " << field.ActionFlag << " UserID: " << field.UserID << " InstrumentID: " << field.InstrumentID
       << " InvestUnitID: " << field.InvestUnitID << " IPAddress: " << field.IPAddress
       << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcInputQuoteActionField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " QuoteActionRef: " << field.QuoteActionRef << " QuoteRef: " << field.QuoteRef
       << " RequestID: " << field.RequestID << " FrontID: " << field.FrontID << " SessionID: " << field.SessionID
       << " ExchangeID: " << field.ExchangeID << " QuoteSysID: " << field.QuoteSysID
       << " ActionFlag: " << field.ActionFlag << " UserID: " << field.UserID << " InstrumentID: " << field.InstrumentID
       << " InvestUnitID: " << field.InvestUnitID << " ClientID: " << field.ClientID
       << " IPAddress: " << field.IPAddress << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcOrderField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " OrderRef: " << field.OrderRef << " UserID: " << field.UserID
       << " OrderPriceType: " << field.OrderPriceType << " Direction: " << ctp_direction(field.Direction)
       << " CombOffsetFlag: " << field.CombOffsetFlag << " CombHedgeFlag: " << field.CombHedgeFlag
       << " LimitPrice: " << field.LimitPrice << " VolumeTotalOriginal: " << field.VolumeTotalOriginal
       << " TimeCondition: " << ctp_time_condition(field.TimeCondition) << " GTDDate: " << field.GTDDate
       << " VolumeCondition: " << ctp_volume_condition(field.VolumeCondition) << " MinVolume: " << field.MinVolume
       << " ContingentCondition: " << ctp_contingent_condition(field.ContingentCondition)
       << " StopPrice: " << field.StopPrice << " ForceCloseReason: " << ctp_force_close_reason(field.ForceCloseReason)
       << " IsAutoSuspend: " << field.IsAutoSuspend << " BusinessUnit: " << field.BusinessUnit
       << " RequestID: " << field.RequestID << " OrderLocalID: " << field.OrderLocalID
       << " ExchangeID: " << field.ExchangeID << " ParticipantID: " << field.ParticipantID
       << " ClientID: " << field.ClientID << " ExchangeInstID: " << field.ExchangeInstID
       << " TraderID: " << field.TraderID << " InstallID: " << field.InstallID
       << " OrderSubmitStatus: " << ctp_order_submit_status(field.OrderSubmitStatus)
       << " NotifySequence: " << field.NotifySequence << " TradingDay: " << field.TradingDay
       << " SettlementID: " << field.SettlementID << " OrderSysID: " << field.OrderSysID
       << " OrderSource: " << field.OrderSource << " OrderStatus: " << ctp_order_status(field.OrderStatus)
       << " OrderType: " << ctp_order_type(field.OrderType) << " VolumeTraded: " << field.VolumeTraded
       << " VolumeTotal: " << field.VolumeTotal << " InsertDate: " << field.InsertDate
       << " InsertTime: " << field.InsertTime << " ActiveTime: " << field.ActiveTime
       << " SuspendTime: " << field.SuspendTime << " UpdateTime: " << field.UpdateTime
       << " CancelTime: " << field.CancelTime << " ActiveTraderID: " << field.ActiveTraderID
       << " ClearingPartID: " << field.ClearingPartID << " SequenceNo: " << field.SequenceNo
       << " FrontID: " << field.FrontID << " SessionID: " << field.SessionID
       << " UserProductInfo: " << field.UserProductInfo << " StatusMsg: " << gbk2utf8((char*)field.StatusMsg)
       << " UserForceClose: " << field.UserForceClose << " ActiveUserID: " << field.ActiveUserID
       << " BrokerOrderSeq: " << field.BrokerOrderSeq << " RelativeOrderSysID: " << field.RelativeOrderSysID
       << " ZCETotalTradedVolume: " << field.ZCETotalTradedVolume << " IsSwapOrder: " << field.IsSwapOrder
       << " BranchID: " << field.BranchID << " InvestUnitID: " << field.InvestUnitID
       << " AccountID: " << field.AccountID << " CurrencyID: " << field.CurrencyID << " IPAddress: " << field.IPAddress
       << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcExecOrderField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " ExecOrderRef: " << field.ExecOrderRef
       << " UserID: " << field.UserID << " Volume: " << field.Volume << " RequestID: " << field.RequestID
       << " BusinessUnit: " << field.BusinessUnit << " OffsetFlag: " << field.OffsetFlag
       << " HedgeFlag: " << field.HedgeFlag << " ActionType: " << field.ActionType
       << " PosiDirection: " << ctp_position_direction(field.PosiDirection)
       << " ReservePositionFlag: " << field.ReservePositionFlag << " CloseFlag: " << field.CloseFlag
       << " ExecOrderLocalID: " << field.ExecOrderLocalID << " ExchangeID: " << field.ExchangeID
       << " ParticipantID: " << field.ParticipantID << " ClientID: " << field.ClientID
       << " ExchangeInstID: " << field.ExchangeInstID << " TraderID: " << field.TraderID
       << " InstallID: " << field.InstallID
       << " OrderSubmitStatus: " << ctp_order_submit_status(field.OrderSubmitStatus)
       << " NotifySequence: " << field.NotifySequence << " TradingDay: " << field.TradingDay
       << " SettlementID: " << field.SettlementID << " ExecOrderSysID: " << field.ExecOrderSysID
       << " InsertDate: " << field.InsertDate << " InsertTime: " << field.InsertTime
       << " CancelTime: " << field.CancelTime << " ExecResult: " << field.ExecResult
       << " ClearingPartID: " << field.ClearingPartID << " SequenceNo: " << field.SequenceNo
       << " FrontID: " << field.FrontID << " SessionID: " << field.SessionID
       << " UserProductInfo: " << field.UserProductInfo << " StatusMsg: " << gbk2utf8((char*)field.StatusMsg)
       << " ActiveUserID: " << field.ActiveUserID << " BrokerExecOrderSeq: " << field.BrokerExecOrderSeq
       << " BranchID: " << field.BranchID << " InvestUnitID: " << field.InvestUnitID
       << " AccountID: " << field.AccountID << " CurrencyID: " << field.CurrencyID << " IPAddress: " << field.IPAddress
       << " MacAddress: " << field.MacAddress;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcTradeField& field) {
    os << "BrokerID: " << field.BrokerID << " InvestorID: " << field.InvestorID
       << " InstrumentID: " << field.InstrumentID << " OrderRef: " << field.OrderRef << " UserID: " << field.UserID
       << " ExchangeID: " << field.ExchangeID << " TradeID: " << field.TradeID
       << " Direction: " << ctp_direction(field.Direction) << " OrderSysID: " << field.OrderSysID
       << " ParticipantID: " << field.ParticipantID << " ClientID: " << field.ClientID
       << " TradingRole: " << field.TradingRole << " ExchangeInstID: " << field.ExchangeInstID
       << " OffsetFlag: " << field.OffsetFlag << " HedgeFlag: " << field.HedgeFlag << " Price: " << field.Price
       << " Volume: " << field.Volume << " TradeDate: " << field.TradeDate << " TradeTime: " << field.TradeTime
       << " TradeType: " << field.TradeType << " PriceSource: " << field.PriceSource << " TraderID: " << field.TraderID
       << " OrderLocalID: " << field.OrderLocalID << " ClearingPartID: " << field.ClearingPartID
       << " BusinessUnit: " << field.BusinessUnit << " SequenceNo: " << field.SequenceNo
       << " TradingDay: " << field.TradingDay << " SettlementID: " << field.SettlementID
       << " BrokerOrderSeq: " << field.BrokerOrderSeq << " TradeSource: " << field.TradeSource;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcExchangeField& field) {
    os << "ExchangeID: " << field.ExchangeID << " ExchangeName: " << gbk2utf8((char*)field.ExchangeName)
       << " ExchangeProperty: " << field.ExchangeProperty;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const CThostFtdcProductField& field) {
    os << "ProductID: " << field.ProductID << " ProductName: " << gbk2utf8((char*)field.ProductName)
       << " ExchangeID: " << field.ExchangeID << " ProductClass: " << ctp_product_class(field.ProductClass)
       << " VolumeMultiple: " << field.VolumeMultiple << " PriceTick: " << field.PriceTick
       << " MaxMarketOrderVolume: " << field.MaxMarketOrderVolume
       << " MinMarketOrderVolume: " << field.MinMarketOrderVolume
       << " MaxLimitOrderVolume: " << field.MaxLimitOrderVolume << " MinLimitOrderVolume: " << field.MinLimitOrderVolume
       << " PositionType: " << ctp_position_type(field.PositionType)
       << " PositionDateType: " << ctp_position_date_type(field.PositionDateType)
       << " CloseDealType: " << ctp_close_deal_type(field.CloseDealType)
       << " TradeCurrencyID: " << field.TradeCurrencyID
       << " MortgageFundUseRange: " << ctp_mortgage_range_type(field.MortgageFundUseRange)
       << " ExchangeProductID: " << field.ExchangeProductID << " UnderlyingMultiple: " << field.UnderlyingMultiple;
    return os;
}

inline string ctp_result(int iResult) { return (iResult == 0) ? "success" : "failed"; }

#endif
