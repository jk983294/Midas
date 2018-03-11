#include <utils/log/Log.h>
#include "MySymbol.h"

using namespace midas;
using namespace std;

uint32_t MySymbol::snap() {
    if (!book.myAlloc) {
        book.bidLevels = new midas::BidBookLevel[book.numBidLevels];
        book.askLevels = new midas::AskBookLevel[book.numAskLevels];
    }

    uint32_t ret;
    if ((ret = sub->snap(BOOK_SIDE_BOTH, &book)) == CONSUMER_OK) print_book();

    if (!book.myAlloc) {
        delete[] book.bidLevels;
        delete[] book.askLevels;
    }
    return ret;
}

void MySymbol::print_book() const {
    MIDAS_LOG_INFO(name << " level1 bid: " << book.bidLevels[0].price << " " << book.bidLevels[0].shares
                        << " ask: " << book.askLevels[0].price << " " << book.askLevels[0].shares);
}
