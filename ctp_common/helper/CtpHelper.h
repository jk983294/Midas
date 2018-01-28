#ifndef MIDAS_CTP_HELPER_H
#define MIDAS_CTP_HELPER_H

#include <memory>
#include <string>
#include "model/CtpInstrument.h"

string get_product_name(const string& instrumentName);

void set_master_contract(CtpInstrument& instrument);

void set_master_contract(map<string, std::shared_ptr<CtpInstrument>>& instruments);

void set_strategy(map<string, std::shared_ptr<CtpInstrument>>& instruments);

#endif
