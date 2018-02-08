#include "CandleData.h"
#include "utils/convert/TimeHelper.h"

// au -> 2100 <-> 230 330 900 <-> 1015 75 1030 <-> 1130 60 1330 <-> 1500 90 total minute: 555
constexpr int maxOneDayMinute = 555;

CandleData::CandleData(int date, int time, double open, double high, double low, double close, double volume)
    : timestamp(date, time), open(open), high(high), low(low), close(close), volume(volume) {}

void CandleData::update_first_tick(int _date, int _intradayMinute, double tp, int ts, double newHigh, double newLow) {
    timestamp.init_from_intraday_minute(_date, _intradayMinute);
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

void CandleData::update(const CandleData& data) {
    tickCount += data.tickCount;
    high = std::max(high, data.high);
    low = std::min(low, data.low);
    close = data.close;
    volume += data.volume;
}

Candles::Candles(CandleScale s) : scale{s} {
    vector<CandleData> v;
    init(v);
}

void Candles::init(const vector<CandleData>& historicData, CandleScale historicScale) {
    data.clear();
    if (scale == historicScale) {
        data.resize(historicData.size());
        std::copy(historicData.begin(), historicData.end(), data.begin());
    } else if (historicScale < scale) {
        for (const auto& candle : historicData) {
            if (sessions.update_session(candle.timestamp.intradayMinute)) {
                int startIntradayMinute = sessions.adjust_within_session(candle.timestamp.intradayMinute, scale);
                if (data.empty() || (startIntradayMinute != (data.end() - 1)->timestamp.intradayMinute)) {
                    data.push_back(candle);
                    (data.end() - 1)->timestamp.intradayMinute = startIntradayMinute;
                    (data.end() - 1)->timestamp.time = midas::Timestamp::intraday_minute2hms(startIntradayMinute);
                } else {
                    (data.end() - 1)->update(candle);
                }
            }
        }
    } else {
        // do nothing because cannot build candle due to high resolution info missing
    }

    historicDataCount = static_cast<int>(data.size());
    totalBinCount = historicDataCount + maxOneDayMinute / scale + 5;
    data.resize(static_cast<size_t>(totalBinCount));
    currentBinIndex = historicDataCount;
}

void Candles::update(const CThostFtdcDepthMarketDataField& tick, int ts, double newHigh, double newLow) {
    int intradayMinute = TradeSessions::intraday_minute(tick.UpdateTime);
    if (sessions.update_session(intradayMinute)) {  // in trading hour
        ++updateCount;

        int date = midas::cob(tick.ActionDay);
        int startIntradayMinute = (intradayMinute - (intradayMinute % scale));
        startIntradayMinute = sessions.adjust_within_session(startIntradayMinute, scale);
        if (data[currentBinIndex].timestamp.cob == 0) {
            data[currentBinIndex].update_first_tick(date, startIntradayMinute, tick.LastPrice, ts, newHigh, newLow);
        } else if (data[currentBinIndex].timestamp.intradayMinute != startIntradayMinute) {
            ++currentBinIndex;
            data[currentBinIndex].update_first_tick(date, startIntradayMinute, tick.LastPrice, ts, newHigh, newLow);
        } else {
            data[currentBinIndex].update_tick(tick.LastPrice, ts, newHigh, newLow);
        }
    }
}

ostream& operator<<(ostream& os, const CandleData& candle) {
    os << candle.timestamp.cob2string() << "," << candle.timestamp.time2string() << "," << candle.open << ","
       << candle.high << "," << candle.low << "," << candle.close << "," << candle.volume;
    return os;
}
ostream& operator<<(ostream& os, const Candles& candles) {
    os << "date,time,open,high,low,close,volume\n";

    int totalAvailableCount = candles.total_available_count();
    for (int i = 0; i < totalAvailableCount; ++i) {
        os << candles.data[i] << '\n';
    }
    return os;
}
