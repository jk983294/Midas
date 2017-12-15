#include <dao/DaoManager.h>
#include <iostream>

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
    return 0;
}
