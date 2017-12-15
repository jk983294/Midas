#ifndef MIDAS_DAO_MANAGER_H
#define MIDAS_DAO_MANAGER_H

#include <memory>
#include "CandleDao.h"
#include "InstrumentInfoDao.h"
#include "MysqlCommon.h"
#include "midas/Singleton.h"

using namespace std;

class DaoManager {
public:
    sql::mysql::MySQL_Driver *driver;
    std::shared_ptr<CandleDao> candleDao;
    std::shared_ptr<InstrumentInfoDao> instrumentInfoDao;
    MysqlAccount account;

public:
    /**
     * get_mysql_driver_instance not thread safe, make it Singleton,
     * so no other where call this function
     */
    DaoManager() {
        driver = sql::mysql::get_mysql_driver_instance();
        candleDao = std::make_shared<CandleDao>(*driver, account);
        instrumentInfoDao = std::make_shared<InstrumentInfoDao>(*driver, account);
    }

    static DaoManager &instance() { return midas::Singleton<DaoManager>::instance(); }

    bool test_connection() {
        bool good{true};
        try {
            std::shared_ptr<sql::Connection> connection{
                driver->connect(account.ip, account.userName, account.password)};
            good = connection->isValid();
        } catch (sql::SQLException &e) {
            good = false;
            cout << e << endl;
        }
        return good;
    }
};

#endif
