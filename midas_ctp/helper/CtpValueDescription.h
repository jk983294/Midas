#ifndef MIDAS_CTP_VALUE_DESC_H
#define MIDAS_CTP_VALUE_DESC_H

#include <ctp/ThostFtdcUserApiStruct.h>
#include <string>

using namespace std;

static string ctp_default_value{"null"};

inline string& ctp_order_source(TThostFtdcOrderSourceType orderSource) {
    static string desc[]{"participant", "administrator"};
    return desc[orderSource - THOST_FTDC_OSRC_Participant];
}

inline string& ctp_position_direction(TThostFtdcPosiDirectionType direction) {
    static string desc[]{"net", "long", "short"};
    return desc[direction - THOST_FTDC_PD_Net];
}

inline string& ctp_position_type(TThostFtdcPositionTypeType type) {
    static string desc[]{"net", "gross"};
    return desc[type - THOST_FTDC_PT_Net];
}

inline string& ctp_direction(TThostFtdcDirectionType direction) {
    static string desc[]{"buy", "sell"};
    return desc[direction - THOST_FTDC_D_Buy];
}

inline string& ctp_time_condition(TThostFtdcTimeConditionType timeCondition) {
    // 立即完成，否则撤销 / 本节有效 / 当日有效 / 指定日期前有效 / 撤销前有效 / 集合竞价有效
    static string desc[]{"IOC", "GFS", "GFD", "GTD", "GTC", "GFA"};
    return desc[timeCondition - THOST_FTDC_TC_IOC];
}

inline string& ctp_product_class(TThostFtdcProductClassType productClass) {
    static string desc[]{"Futures", "Options", "Combination", "Spot", "EFP", "SpotOption", "ETFOption", "Stock"};
    return desc[productClass - THOST_FTDC_PC_Futures];
}

inline string& ctp_volume_condition(TThostFtdcVolumeConditionType volumeCondition) {
    // 任何数量 / 最小数量 / 全部数量
    static string desc[]{"any", "min", "complete"};
    return desc[volumeCondition - THOST_FTDC_VC_AV];
}

inline string& ctp_contingent_condition(TThostFtdcContingentConditionType contingentCondition) {
    static string desc1[]{
        "Immediately",                     // 立即
        "Touch",                           // 止损
        "TouchProfit",                     // 止赢
        "ParkedOrder",                     // 预埋单
        "LastPriceGreaterThanStopPrice",   // 最新价大于条件价
        "LastPriceGreaterEqualStopPrice",  // 最新价大于等于条件价
        "LastPriceLesserThanStopPrice",    // 最新价小于条件价
        "LastPriceLesserEqualStopPrice",   // 最新价小于等于条件价
        "AskPriceGreaterThanStopPrice"     // 卖一价大于条件价
    };
    static string desc2[]{
        "apGreaterEqualStopPrice",  // 卖一价大于等于条件价
        "apLesserThanStopPrice",    // 卖一价小于条件价
        "apLesserEqualStopPrice",   // 卖一价小于等于条件价
        "bpGreaterThanStopPrice",   // 买一价大于条件价
        "bpGreaterEqualStopPrice",  // 买一价大于等于条件价
        "bpLesserThanStopPrice"     // 买一价小于条件价
    };
    static string desc3{"bpLesserEqualStopPrice"};  // 买一价小于等于条件价
    if (contingentCondition >= THOST_FTDC_CC_Immediately &&
        contingentCondition <= THOST_FTDC_CC_AskPriceGreaterThanStopPrice) {
        return desc1[contingentCondition - THOST_FTDC_CC_Immediately];
    } else if (contingentCondition >= THOST_FTDC_CC_AskPriceGreaterEqualStopPrice &&
               contingentCondition <= THOST_FTDC_CC_BidPriceLesserThanStopPrice) {
        return desc2[contingentCondition - THOST_FTDC_CC_AskPriceGreaterEqualStopPrice];
    } else if (contingentCondition == THOST_FTDC_CC_BidPriceLesserEqualStopPrice) {
        return desc3;
    }
    return ctp_default_value;
}

inline string& ctp_order_status(TThostFtdcOrderStatusType orderStatus) {
    // 全部成交 / 部分成交还在队列中 / 分成交不在队列中 / 未成交还在队列中 / 未成交不在队列中 / 撤单
    static string desc1[]{"AllTraded",       "PartTradedQueueing", "PartTradedNotQueueing",
                          "NoTradeQueueing", "NoTradeNotQueueing", "Canceled"};
    // 未知 / 尚未触发 / 已触发
    static string desc2[]{"Unknown", "NotTouched", "Touched"};
    if (orderStatus >= THOST_FTDC_OST_AllTraded && orderStatus <= THOST_FTDC_OST_Canceled) {
        return desc1[orderStatus - THOST_FTDC_OST_AllTraded];
    } else {
        return desc2[orderStatus - THOST_FTDC_OST_Unknown];
    }
}

inline string& ctp_force_close_reason(TThostFtdcForceCloseReasonType forceCloseReason) {
    static string desc[]{
        "NotForceClose",            // 非强平
        "LackDeposit",              // 资金不足
        "ClientOverPositionLimit",  // 客户超仓
        "MemberOverPositionLimit",  // 会员超仓
        "NotMultiple",              // 持仓非整数倍
        "Violation",                // 违规
        "Other",                    // 其它
        "PersonDeliver"             // 自然人临近交割
    };
    return desc[forceCloseReason - THOST_FTDC_FCC_NotForceClose];
}

inline string& ctp_order_submit_status(TThostFtdcOrderSubmitStatusType orderSubmitStatus) {
    // 已经提交 / 撤单已经提交 / 修改已经提交 / 已经接受 / 报单已经被拒绝 / 撤单已经被拒绝 / 改单已经被拒绝
    static string desc[]{"InsertSubmitted", "CancelSubmitted", "ModifySubmitted", "Accepted",
                         "InsertRejected",  "CancelRejected",  "ModifyRejected"};
    return desc[orderSubmitStatus - THOST_FTDC_OSS_InsertSubmitted];
}

inline string& ctp_order_type(TThostFtdcOrderTypeType orderType) {
    // 正常 / 报价衍生 / 组合衍生 / 组合报单 / 条件单 / 互换单
    static string desc[]{"Normal",      "DeriveFromQuote",  "DeriveFromCombination",
                         "Combination", "ConditionalOrder", "Swap"};
    return desc[orderType - THOST_FTDC_ORDT_Normal];
}

#endif
