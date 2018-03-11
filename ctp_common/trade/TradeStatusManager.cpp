#include "TradeStatusManager.h"
#include "helper/CtpHelper.h"
#include "utils/FileUtils.h"
#include "utils/VisualHelper.h"

void TradeStatusManager::load_trade_session(const string& path) {
    if (midas::check_file_exists(path.c_str())) {
        ifstream ifs(path, ifstream::in);
        if (ifs.is_open()) {
            string s;
            while (getline(ifs, s)) {
                if (!s.empty()) {
                    size_t itr1 = s.find_first_of(' ');
                    string productType = s.substr(0, itr1);
                    size_t itr2;
                    while ((itr2 = s.find_first_of(',', itr1 + 1)) != string::npos) {
                        product2sessions[productType].add_session(TradeSession{s.substr(itr1 + 1, itr2 - itr1 - 1)});
                        itr1 = itr2;
                    }
                    product2sessions[productType].add_session(TradeSession{s.substr(itr1 + 1)});
                }
            }
            ifs.close();
        }
    } else {
        cerr << "session file not exist" << '\n';
    }
}

const TradeSessions& TradeStatusManager::get_session(const string& instrumentId) {
    static TradeSessions dummy;
    string productId = get_product_name(instrumentId);

    auto itr = product2sessions.find(productId);
    if (itr != product2sessions.end())
        return itr->second;
    else {
        MIDAS_LOG_WARNING("no trade session found for " << instrumentId << " using dummy one");
        product2sessions[productId] = dummy;
        return dummy;
    }
}

ostream& operator<<(ostream& s, const TradeStatusManager& manager) {
    s << manager.product2sessions;
    return s;
}

int TradeSessions::intraday_minute(const char* updateTime) {
    int hour = (updateTime[0] - '0') * 10 + (updateTime[1] - '0');
    int minute = (updateTime[3] - '0') * 10 + (updateTime[4] - '0');
    return 60 * hour + minute;
}
