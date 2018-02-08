#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <unordered_set>
#include "InstrumentInfoDao.h"
#include "utils/convert/TimeHelper.h"
#include "utils/log/Log.h"

using namespace std;

InstrumentInfoDao::InstrumentInfoDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account)
    : driver(_driver), account(_account) {}

std::vector<string> InstrumentInfoDao::get_all_instruments() {
    static const string sql{"select InstrumentID from ctp.instrument_info"};
    vector<string> result;
    try {
        std::shared_ptr<sql::Connection> connection{driver.connect(account.ip, account.userName, account.password)};
        connection->setSchema("ctp");
        std::shared_ptr<sql::Statement> statement{connection->createStatement()};
        std::shared_ptr<sql::ResultSet> resultSet{statement->executeQuery(sql)};

        while (resultSet->next()) {
            result.push_back(resultSet->getString(1));
        }
    } catch (sql::SQLException& e) {
        MIDAS_LOG_ERROR("get_all_instruments error " << e);
    }
    return result;
}

int InstrumentInfoDao::save_instruments(const vector<std::shared_ptr<CThostFtdcInstrumentField>>& instruments) {
    static const string sql{
        "insert into ctp.instrument_info (InstrumentID, ExchangeID, VolumeMultiple, PriceTick, CreateDate, OpenDate, "
        "ExpireDate, LongMarginRatio, ShortMarginRatio, UnderlyingMultiple) values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"};

    vector<string> alreadyInDb = get_all_instruments();
    unordered_set<std::string> alreadyInDbInstruments(alreadyInDb.begin(), alreadyInDb.end());

    int saved = 0;
    try {
        std::shared_ptr<sql::Connection> connection{driver.connect(account.ip, account.userName, account.password)};
        connection->setSchema("ctp");
        std::shared_ptr<sql::PreparedStatement> statement{connection->prepareStatement(sql)};

        for (const auto& pItem : instruments) {
            CThostFtdcInstrumentField& item = *pItem;
            if (alreadyInDbInstruments.find(item.InstrumentID) != alreadyInDbInstruments.end()) {
                continue;
            }

            statement->setString(1, item.InstrumentID);
            statement->setString(2, item.ExchangeID);
            statement->setInt(3, item.VolumeMultiple);
            statement->setDouble(4, item.PriceTick);
            statement->setInt(5, midas::cob(item.CreateDate));
            statement->setInt(6, midas::cob(item.OpenDate));
            statement->setInt(7, midas::cob(item.ExpireDate));
            statement->setDouble(8, item.LongMarginRatio);
            statement->setDouble(9, item.ShortMarginRatio);
            statement->setDouble(10, item.UnderlyingMultiple);

            statement->execute();
            ++saved;
        }
    } catch (sql::SQLException& e) {
        MIDAS_LOG_ERROR("save_instruments error " << e);
    }
    return saved;
}
