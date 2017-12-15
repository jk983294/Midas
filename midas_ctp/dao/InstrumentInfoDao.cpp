#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "InstrumentInfoDao.h"

InstrumentInfoDao::InstrumentInfoDao(sql::mysql::MySQL_Driver& _driver, const MysqlAccount& _account)
    : driver(_driver), account(_account) {}

std::vector<string> InstrumentInfoDao::get_all_instruments() {
    vector<string> result;
    try {
        std::shared_ptr<sql::Connection> connection{driver.connect(account.ip, account.userName, account.password)};
        connection->setSchema("ctp");
        std::shared_ptr<sql::Statement> statement{connection->createStatement()};
        std::shared_ptr<sql::ResultSet> resultSet{statement->executeQuery("SELECT InstrumentID FROM instrument_info")};

        while (resultSet->next()) {
            result.push_back(resultSet->getString(1));
        }
    } catch (sql::SQLException& e) {
        cout << e << endl;
    }
    return result;
}

int InstrumentInfoDao::save_instruments(const std::vector<CThostFtdcInstrumentField>& instruments) {
    vector<string> alreadyInDb = get_all_instruments();
    return 0;
}
