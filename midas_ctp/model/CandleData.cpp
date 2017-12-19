#include "CandleData.h"
#include "utils/convert/TimeHelper.h"

// au -> 2100 <-> 230 330 900 <-> 1015 75 1030 <-> 1130 60 1330 <-> 1500 90 total minute: 555
constexpr int maxOneDayMinute = 555;

CandleData::CandleData(int date, int time, double open, double high, double low, double close, double volume)
    : date(date), time(time), open(open), high(high), low(low), close(close), volume(volume) {}

void CandleData::update_first_tick(int _date, int _intradayMinute, double tp, int ts, double newHigh, double newLow) {
    date = _date;
    intradayMinute = _intradayMinute;
    time = (_intradayMinute % 60 + (_intradayMinute / 60) * 100) * 100;
    volume = ts;
    open = close = high = low = tp;
    if (newHigh > 0) high = std::max(high, newHigh);
    if (newLow > 0) low = std::min(low, newLow);
    tickCount = 1;
}

void CandleData::update_tick(double tp, int ts, double newHigh, double newLow) {
    high = std::max(high, tp);
    low = std::min(low, tp);
    if (newHigh > 0) high = std::max(high, newHigh);
    if (newLow > 0) low = std::min(low, newLow);
    close = tp;
    volume += ts;
    ++tickCount;
}

int CandleData::time_diff(int newDate, int newIntradayMinute) {
    return (newDate - date) * 60 * 24 + (newIntradayMinute - intradayMinute);
}

Candles::Candles(CandleScale s) : scale{s} {
    vector<CandleData> v;
    init(v);
}

void Candles::init(vector<CandleData>& historicData) {
    if (historicData.size()) {
        std::swap(data, historicData);
    }

    historicDataCount = data.size();
    totalBinCount = historicDataCount + maxOneDayMinute / scale + 5;
    data.resize(totalBinCount);
    currentBinIndex = historicDataCount;
}

void Candles::update(const CThostFtdcDepthMarketDataField& tick, int ts, double newHigh, double newLow) {
    int intradayMinute = TradeSessions::intraday_minute(tick.UpdateTime);
    if (sessions.update_session(intradayMinute)) {  // in trading hour
        ++updateCount;

        int date = midas::cob(tick.ActionDay);
        int startIntradayMinute = (intradayMinute - (intradayMinute % scale));
        startIntradayMinute = sessions.adjust_within_session(startIntradayMinute, scale);
        if (data[currentBinIndex].tickCount == 0) {
            data[currentBinIndex].update_first_tick(date, startIntradayMinute, tick.LastPrice, ts, newHigh, newLow);
        } else if (data[currentBinIndex].intradayMinute != startIntradayMinute) {
            ++currentBinIndex;
            data[currentBinIndex].update_first_tick(date, startIntradayMinute, tick.LastPrice, ts, newHigh, newLow);
        } else {
            data[currentBinIndex].update_tick(tick.LastPrice, ts, newHigh, newLow);
        }
    }
}

ostream& operator<<(ostream& os, const CandleData& candle) {
    os << candle.date << "," << candle.time << "," << candle.open << "," << candle.high << "," << candle.low << ","
       << candle.close << "," << candle.volume << "," << candle.tickCount;
    return os;
}
ostream& operator<<(ostream& os, const Candles& candles) {
    os << "date,time,open,high,low,close,volume,tickCount" << '\n';
    for (size_t i = 0; i < candles.currentBinIndex + 1; ++i) {
        os << candles.data[i] << '\n';
    }
    return os;
}
