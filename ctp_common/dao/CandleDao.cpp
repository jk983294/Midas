#include <cppconn/prepared_statement.h>
#include <mysql_connection.h>
#include "CandleDao.h"
#include "utils/convert/TimeHelper.h"
#include "utils/log/Log.h"

CandleDao::CandleDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account)
    : driver(_driver), account(_account) {}

void CandleDao::get_all_candles(std::unordered_map<std::string, std::vector<CandleData>>& data) {
    static const string sql{
        "select instrument, time, open, high, low, close, volume from ctp.candle15 order by instrument, time"};

    vector<string> result;
    try {
        std::shared_ptr<sql::Connection> connection{driver.connect(account.ip, account.userName, account.password)};
        connection->setSchema("ctp");
        std::shared_ptr<sql::Statement> statement{connection->createStatement()};
        std::shared_ptr<sql::ResultSet> resultSet{statement->executeQuery(sql)};

        while (resultSet->next()) {
            string instrument = resultSet->getString(1);
            string timeStr = resultSet->getString(2);
            int cob = midas::cob_from_dash(timeStr.c_str());
            int time = midas::intraday_time_HMS(timeStr.c_str() + 11);
            double open = static_cast<double>(resultSet->getDouble(3));
            double high = static_cast<double>(resultSet->getDouble(4));
            double low = static_cast<double>(resultSet->getDouble(5));
            double close = static_cast<double>(resultSet->getDouble(6));
            double volume = static_cast<double>(resultSet->getDouble(7));
            data[instrument].push_back({cob, time, open, high, low, close, volume});
        }
    } catch (sql::SQLException& e) {
        MIDAS_LOG_ERROR("get_all_candles error " << e.what());
    }
}

int CandleDao::save_candles(const std::string& instrument, const std::vector<CandleData>& data, size_t pos) {
    static const string sql{
        "insert into ctp.candle15 (instrument, time, open, high, low, close, volume) values (?, ?, ?, ?, ?, ?, ?)"};

    int saved = 0;
    try {
        std::shared_ptr<sql::Connection> connection{driver.connect(account.ip, account.userName, account.password)};
        connection->setSchema("ctp");
        std::shared_ptr<sql::PreparedStatement> statement{connection->prepareStatement(sql)};

        for (auto itr = data.begin() + pos; itr != data.end(); ++itr) {
            if (itr->tickCount == 0) break;

            statement->setString(1, instrument);
            statement->setString(2, itr->timestamp.to_string());
            statement->setDouble(3, itr->open);
            statement->setDouble(4, itr->high);
            statement->setDouble(5, itr->low);
            statement->setDouble(6, itr->close);
            statement->setDouble(7, itr->volume);

            statement->execute();
            ++saved;
        }
    } catch (sql::SQLException& e) {
        MIDAS_LOG_ERROR("save_candles error " << e.what());
    }
    return saved;
}
