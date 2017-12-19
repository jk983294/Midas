#include <cstring>
#include "dao/DaoManager.h"

using namespace std;

int main(int argc, char* argv[]) {
    DaoManager::instance().account.ip = "tcp://127.0.0.1:3306";
    DaoManager::instance().account.userName = "root";
    DaoManager::instance().account.password = "123456";

    if (!DaoManager::instance().test_connection()) {
        cout << "connection failed.\n";
    } else {
        cout << "connection good.\n";
    }

    DaoManager::instance().instrumentInfoDao->get_all_instruments();

    CThostFtdcInstrumentField instrument;
    strcpy(instrument.InstrumentID, "CF711");
    strcpy(instrument.ExchangeID, "CZCE");
    instrument.VolumeMultiple = 5;
    instrument.PriceTick = 5;
    strcpy(instrument.CreateDate, "20161014");
    strcpy(instrument.OpenDate, "20161115");
    strcpy(instrument.ExpireDate, "20171114");
    instrument.LongMarginRatio = 0.05;
    instrument.ShortMarginRatio = 0.05;
    instrument.UnderlyingMultiple = 1;
    int ret = DaoManager::instance().instrumentInfoDao->save_instruments({instrument});
    cout << "insert " << ret << " entries.\n";

    CandleData candle{20171113, 90000, 53480, 53550, 53370, 53400, 5764};
    vector<CandleData> data;
    data.push_back(candle);
    DaoManager::instance().candleDao->save_candles("cu1712", data, 0);

    unordered_map<string, vector<CandleData>> result;
    DaoManager::instance().candleDao->get_all_candles(result);
    return 0;
}
