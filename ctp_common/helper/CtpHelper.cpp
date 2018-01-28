#include "CtpHelper.h"

string get_product_name(const string& instrumentName) {
    string productName{""};
    for (auto i = instrumentName.begin(); i != instrumentName.end(); ++i) {
        if (*i <= '9' && *i >= '0')
            break;
        else
            productName += *i;
    }
    return productName;
}

void set_master_contract(CtpInstrument& instrument) { instrument.isMasterContract = true; }

void set_master_contract(map<string, std::shared_ptr<CtpInstrument>>& instruments) {
    map<string, vector<string>> product2contracts;
    for (const auto& item : instruments) {
        product2contracts[item.second->productName].push_back(item.first);
    }

    for (auto& item : product2contracts) {
        std::sort(item.second.begin(), item.second.end());

        int index = 0;
        if (item.second.size() >= 3) {
            index = 2;
        }
        set_master_contract(*instruments[item.second[index]]);
    }
}

void set_strategy(map<string, std::shared_ptr<CtpInstrument>>& instruments) {
    for (auto& item : instruments) {
        if (item.second->isMasterContract) {
        }
    }
}
