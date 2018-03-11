#ifndef MIDAS_MYSYMBOL_H
#define MIDAS_MYSYMBOL_H

#include <midas/md/MdBook.h>
#include "Subscription.h"

struct MySymbol {
    MySymbol(std::string const& name, uint16_t exch) : name(name), exchange(exch) {}

    std::string name;
    uint16_t exchange;
    midas::MdBook book;
    Subscription* sub;

    uint32_t snap();

    void print_book() const;
};

#endif
