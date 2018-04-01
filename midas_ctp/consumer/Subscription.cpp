#include <midas/md/MdLock.h>
#include <mutex>
#include "Consumer.h"
#include "Subscription.h"

using namespace std;
using namespace midas;

SubTicker::SubTicker(Consumer& consumer_, std::string const& mdTick_, uint16_t mdExch_, uint8_t* l2AddrBookCache_,
                     uint16_t l2BytesPerProduct_, uint16_t l2OffsetBytesBid_, uint16_t l2OffsetBytesAsk_)
    : consumer(consumer_),
      mdTick(mdTick_),
      mdExch(mdExch_),
      state(State::empty),
      streamId(0),
      locator(UINT16_MAX),
      l2AddrBookCache(l2AddrBookCache_),
      l2BytesPerProduct(l2BytesPerProduct_),
      l2OffsetBytesBid(l2OffsetBytesBid_),
      l2OffsetBytesAsk(l2OffsetBytesAsk_),
      l2LockBid(nullptr),
      l2ShmBookBidMe(nullptr),
      l2LockAsk(nullptr),
      l2ShmBookAskMe(nullptr) {}

void SubTicker::locate(uint16_t code) {
    locator = code;
    init_offsets();
}

void SubTicker::init_offsets() {
    if (l2ShmBookBidMe == nullptr && locator != UINT16_MAX) {
        l2ShmBookBidMe = l2AddrBookCache + locator * l2BytesPerProduct + l2OffsetBytesBid;
        l2LockBid = reinterpret_cast<uint64_t*>(l2AddrBookCache + locator * l2BytesPerProduct +
                                                bid_lock_offset(l2BytesPerProduct));
    }

    if (l2ShmBookAskMe == nullptr && locator != UINT16_MAX) {
        l2ShmBookAskMe = l2AddrBookCache + locator * l2BytesPerProduct + l2OffsetBytesAsk;
        l2LockAsk = reinterpret_cast<uint64_t*>(l2AddrBookCache + locator * l2BytesPerProduct +
                                                ask_lock_offset(l2BytesPerProduct));
    }
}

uint32_t SubTicker::bid_snap(midas::MdBook* book) {
    uint32_t rc = CONSUMER_NO_DATA;
    uint64_t* p = (l2LockBid + 1);
    if (*p == 0) {
        rc = occ_snap(book->bidLevels, l2ShmBookBidMe, sizeof(BidBookLevel) * book->numBidLevels, l2LockBid);
    }
    return rc;
}

uint32_t SubTicker::ask_snap(midas::MdBook* book) {
    uint32_t rc = CONSUMER_NO_DATA;
    uint64_t* p = (l2LockAsk + 1);
    if (*p == 0) {
        rc = occ_snap(book->askLevels, l2ShmBookAskMe, sizeof(AskBookLevel) * book->numAskLevels, l2LockAsk);
    }
    return rc;
}

uint32_t SubTicker::snap(uint8_t side, midas::MdBook* book) {
    uint32_t rc = CONSUMER_NO_DATA;

    if (locator < UINT16_MAX) {
        if (midas::MdBook::BookType::price == book->bookType) {
            switch (side) {
                case BOOK_SIDE_BID: {
                    rc = bid_snap(book);
                } break;
                case BOOK_SIDE_ASK: {
                    rc = ask_snap(book);
                } break;
                case BOOK_SIDE_BOTH: {
                    if ((rc = bid_snap(book)) == CONSUMER_OK) rc = ask_snap(book);
                } break;
                default:
                    break;
            }
        }
    }
    return rc;
}

Subscription* SubTicker::subscription_add(void* userData) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    subscriptions.emplace_back(new Subscription(*this, userData));
    return subscriptions.back().get();
}

bool SubTicker::subscription_delete(Subscription* sub) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
        if (it->get() == sub) {
            subscriptions.erase(it);
            return true;
        }
    }
    return false;
}

void SubTicker::on_book_refreshed(char const* symbol, uint16_t exch) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
        consumer.callback_book_refreshed(symbol, exch, (*it)->userData);
    }
}

void SubTicker::on_book_changed(char const* symbol, uint16_t exch, BookChanged const* bkchngd) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
        consumer.callback_book_changed(symbol, exch, bkchngd, (*it)->userData);
    }
}

void SubTicker::on_trading_action(char const* symbol, uint16_t exch, TradingAction const* trdst) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
        consumer.callback_trade_action(symbol, exch, trdst, (*it)->userData);
    }
}

void SubTicker::on_subscribe_response(char const* symbol, uint16_t exch, uint8_t subscriptionStatus) {
    std::lock_guard<std::recursive_mutex> g(tickerLock);
    for (auto it = subscriptions.begin(); it != subscriptions.end(); ++it) {
        consumer.callback_subscribe_response(symbol, exch, subscriptionStatus, (*it)->userData);
    }
}

uint32_t SubTicker::occ_snap(void* target, uint8_t* source, size_t n, uint64_t* pMeta) {
    const static uint32_t N_Max_Opt_Attempts = consumer.max_opt_snap_attempts;
    const static uint32_t N_Max_Lock_Attempts = consumer.max_lock_snap_attempts;
    const static uint32_t N_Max_Attempts_tsc = consumer.reader_lock_timeout;

    uint32_t nOptAttempts = 0;
    uint32_t nLockAttempts = 0;

    if (!consumer.connected) {
        return CONSUMER_NOT_CONNECTED;
    }

    while (nLockAttempts < N_Max_Lock_Attempts) {
        // loop with max attempts
        while (nOptAttempts < N_Max_Opt_Attempts) {
            // snap the version, long read is atomic on x86 TSO
            uint64_t meta1 = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
            const auto m1 = reinterpret_cast<MdLock::MetaT*>(&meta1);
            const uint64_t nSnappedVersion = m1->version;

            // check versions -
            // writer will increment version twice,
            // step1: odd number to indicate modifying in process and possibly partial data,
            // step2: even number to indicate good data

            // if it's odd version, it's modification in process
            if (nSnappedVersion % 2) {
                continue;
            }

            // just go and grab it - writer may be modifying at the same time
            memcpy(target, source, n);

            // if not modified, good to go
            uint64_t meta2 = __atomic_load_n(pMeta, __ATOMIC_RELAXED);
            const auto m2 = reinterpret_cast<MdLock::MetaT*>(&meta2);
            if (m2->version == nSnappedVersion) {
                return CONSUMER_OK;
            }
            ++nOptAttempts;
        }

        uint64_t ver_1, ver_2;
        if (MdLock::_increment_reader_count(pMeta, consumer.clientId, ver_1, N_Max_Attempts_tsc)) {
            memcpy(target, source, n);
        } else {
            return CONSUMER_TIMED_OUT;
        }
        MdLock::_decrement_reader_count(pMeta, consumer.clientId, ver_2);

        if (ver_1 == ver_2)
            return CONSUMER_OK;
        else
            nOptAttempts = 0;
        ++nLockAttempts;
    }
    return CONSUMER_TIMED_OUT;
}
