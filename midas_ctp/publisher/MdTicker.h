#ifndef MIDAS_MD_TICKER_H
#define MIDAS_MD_TICKER_H

#include <midas/md/MdBook.h>
#include <midas/md/MdDefs.h>
#include <midas/md/MdExchange.h>
#include <midas/md/MdLock.h>
#include <net/raw/MdProtocol.h>
#include <utils/log/Log.h>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include "DataSource.h"

class ConsumerProxy;  // Forward declaration

class MdTicker {
public:
    enum class State : uint8_t {
        empty = 0,   /* Ticker created but not used yet */
        pending = 1, /* Awaiting response from source */
        active = 2,  /* Subscribed */
        badSymbol = 3
    };

public:
    midas::TradingAction tradingAction;
    std::string mdTick;
    DataSource &dataSource;
    uint64_t srcReceiveTime{0};
    uint64_t srcTransmitTime{0};
    uint64_t producerReceiveTime{0};
    midas::BookChanged::ChangedSide bookChangedSide{midas::BookChanged::ChangedSide::none};
    uint16_t locate{UINT16_MAX};
    State state{State::empty};
    uint32_t lotSize{midas::DefaultLotSize};
    uint8_t priceScaleCode{UINT8_MAX};
    uint16_t exchange;
    using bblT = std::unique_ptr<midas::BidBookLevel>;
    using ablT = std::unique_ptr<midas::AskBookLevel>;
    std::vector<bblT> bookBid;
    std::vector<ablT> bookAsk;
    uint16_t bytesPerProduct;
    uint8_t *shmBookAllProducts;
    uint8_t *shmBookBid{nullptr};
    uint8_t shmBookBidLevels;
    uint16_t offsetBytesBid;
    uint64_t *shmLockBid{nullptr};
    uint8_t *shmBookAsk{nullptr};
    uint8_t shmBookAskLevels;
    uint16_t offsetBytesAsk;
    uint64_t *shmLockAsk{nullptr};
    mutable std::mutex tickerLock;
    std::vector<std::shared_ptr<ConsumerProxy>> consumers;
    std::vector<std::shared_ptr<ConsumerProxy>> consumersPendingRefresh;
    uint64_t eventSequence{0};

public:
    MdTicker(DataSource &dataSource, std::string const &mdTick, uint8_t *addrBookCache,
             midas::MdExchange const &exchangeInfo);

    virtual ~MdTicker();

    MdTicker() = delete;

    MdTicker(MdTicker const &) = delete;

    MdTicker(MdTicker &&) = delete;

    MdTicker &operator=(MdTicker const &) = delete;

    MdTicker &operator=(MdTicker &&) = delete;

    void add_subscriber(std::shared_ptr<ConsumerProxy> const &consumer, bool asyncRefresh = false);

    bool remove_subscriber(std::shared_ptr<ConsumerProxy> const &consumer);

    bool is_subscriber(std::shared_ptr<ConsumerProxy> const &consumer);

    bool has_subscriber() const;

    void set_locate(uint16_t code);

    void update_refresh_times(uint64_t srcRecvTime, uint64_t srcXmitTime, uint64_t recvTime);

    void clear_bid();
    void clear_ask();

    void on_subscribe(State state);

    void process_book_refresh_L1();
    void process_book_quote_L1(midas::BookChanged::ChangedSide side);

    void process_book_trading_action(midas::TradingAction::TradeStatus trdst, uint16_t exch);
    void process_book_trading_action(midas::TradingAction::TradeStatus trdst) {
        process_book_trading_action(trdst, exchange);
    }

    void process_book_refresh_bid();
    void process_book_refresh_ask();

    void process_book_update(midas::BookChanged::ChangedSide side, std::size_t numOrigBidLevels,
                             std::size_t numOrigAskLevels);

    bool update_best_bid_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return update_best_price_level(bookBid, price, priceScaleCode, shares, orders, exchTime, exchange, __func__);
    }

    bool update_best_ask_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return update_best_price_level(bookAsk, price, priceScaleCode, shares, orders, exchTime, exchange, __func__);
    }

    bool update_best_bid_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime,
                                     uint16_t exch) {
        return update_best_price_level(bookBid, price, priceScaleCode, shares, orders, exchTime, exch, __func__);
    }

    bool update_best_ask_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime,
                                     uint16_t exch) {
        return update_best_price_level(bookAsk, price, priceScaleCode, shares, orders, exchTime, exch, __func__);
    }

    bool insert_bid_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return insert_price_level(bookBid, price, priceScaleCode, shares, orders, exchTime, producerReceiveTime,
                                  __func__);
    }

    bool insert_ask_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return insert_price_level(bookAsk, price, priceScaleCode, shares, orders, exchTime, producerReceiveTime,
                                  __func__);
    }

    bool insert_bid_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return insert_price_level(bookBid, level, price, priceScaleCode, shares, orders, exchTime, producerReceiveTime,
                                  __func__);
    }

    bool insert_ask_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return insert_price_level(bookAsk, level, price, priceScaleCode, shares, orders, exchTime, producerReceiveTime,
                                  __func__);
    }

    bool modify_bid_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return modify_price_level(bookBid, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool modify_ask_price_level(int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return modify_price_level(bookAsk, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool modify_bid_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return modify_price_level(bookBid, level, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool modify_ask_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return modify_price_level(bookAsk, level, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool fill_bid_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return fill_price_level(bookBid, level, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool fill_ask_price_level(std::size_t level, int64_t price, uint64_t shares, uint32_t orders, uint64_t exchTime) {
        return fill_price_level(bookAsk, level, price, priceScaleCode, shares, orders, exchTime, __func__);
    }

    bool remove_bid_price_level(int64_t price) { return remove_price_level(bookBid, price, __func__); }

    bool remove_ask_price_level(int64_t price) { return remove_price_level(bookAsk, price, __func__); }

    bool delete_bid_price_level(std::size_t level) { return delete_price_level(bookBid, level, __func__); }

    bool delete_ask_price_level(std::size_t level) { return delete_price_level(bookAsk, level, __func__); }

    bool delete_from_bid_price_level(std::size_t level) { return delete_from_price_level(bookBid, level, __func__); }

    bool delete_from_ask_price_level(std::size_t level) { return delete_from_price_level(bookAsk, level, __func__); }

    bool delete_through_bid_price_level(std::size_t level) {
        return delete_through_price_level(bookBid, level, __func__);
    }

    bool delete_through_ask_price_level(std::size_t level) {
        return delete_through_price_level(bookAsk, level, __func__);
    }

    bool reset_bid_price_level() {
        bookBid.clear();
        return true;
    }

    bool reset_ask_price_level() {
        bookAsk.clear();
        return true;
    }

    void send_pending_refresh();
    void cancel_pending_refresh();

protected:
    void init_offsets();

    // common bid / ask book update method refactor
    template <typename Book>
    bool update_best_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares, uint32_t orders,
                                 uint64_t exchTime, uint16_t exch, const char *callFunc);

    template <typename Book>
    bool insert_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares, uint32_t orders,
                            uint64_t exchTime, int64_t recvTime, const char *callFunc);

    template <typename Book>
    bool insert_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                            uint32_t orders, uint64_t exchTime, int64_t recvTime, const char *callFunc);

    template <typename Book>
    bool modify_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares, uint32_t orders,
                            uint64_t exchTime, const char *callFunc);

    template <typename Book>
    bool modify_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                            uint32_t orders, uint64_t exchTime, const char *callFunc);

    template <typename Book>
    bool fill_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                          uint32_t orders, uint64_t exchTime, const char *callFunc);

    template <typename Book>
    bool remove_price_level(Book &book, int64_t price, const char *callFunc);

    template <typename Book>
    bool delete_price_level(Book &book, std::size_t level, const char *callFunc);

    template <typename Book>
    bool delete_from_price_level(Book &book, std::size_t level, const char *callFunc);

    template <typename Book>
    bool delete_through_price_level(Book &book, std::size_t level, const char *callFunc);

private:
    midas::BidBookLevel blank_bid_level() const;

    midas::AskBookLevel blank_ask_level() const;
};

// Called from market data thread only
template <typename Book>
bool MdTicker::update_best_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                                       uint32_t orders, uint64_t exchTime, uint16_t exch, const char * /*callFunc*/) {
    if (book.empty()) {
        auto newLevel(new typename Book::value_type::element_type);
        book.emplace_back(std::move(newLevel));
    }

    auto &level = book[0];
    level->exchange = exch;
    level->lotSize = lotSize;
    level->orders = orders;
    level->price = price;
    level->priceScaleCode = priceScaleCode;
    level->sequence = eventSequence;
    level->shares = shares;
    level->timestamp = exchTime;
    level->updateTS = producerReceiveTime;
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::insert_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares, uint32_t orders,
                                  uint64_t exchTime, int64_t recvTime, const char *callFunc) {
    auto pos = std::lower_bound(book.begin(), book.end(), price);
    if (pos != book.end() && (*pos)->price == price) {
        return false;
    }

    auto level(new typename Book::value_type::element_type);
    level->exchange = exchange;
    level->lotSize = lotSize;
    level->orders = orders;
    level->price = price;
    level->priceScaleCode = priceScaleCode;
    level->sequence = eventSequence;
    level->shares = shares;
    level->timestamp = exchTime;
    level->updateTS = recvTime;
    book.emplace(pos, std::move(level));
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::insert_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                                  uint32_t orders, uint64_t exchTime, int64_t recvTime, const char *callFunc) {
    if (level > book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range in " << level);
        return false;
    }

    auto pos = book.end();
    if (level == book.size()) {
        if (pos != book.begin() && !(*std::prev(pos) < price)) {
            MIDAS_LOG_WARNING(callFunc << " price is out of order " << price);
            return false;
        }
    } else {
        pos = std::next(book.begin(), static_cast<typename decltype(book.begin())::difference_type>(level));
        if ((*pos)->price == price) {
            MIDAS_LOG_WARNING(callFunc << " price already exists " << price);
            return false;
        }
        if ((pos != book.begin() && !(*std::prev(pos) < price)) || !(price < *pos)) {
            MIDAS_LOG_WARNING(callFunc << " price is out of order " << price);
            return false;
        }
    }

    auto bookLevel(new typename Book::value_type::element_type);
    bookLevel->exchange = exchange;
    bookLevel->lotSize = lotSize;
    bookLevel->orders = orders;
    bookLevel->price = price;
    bookLevel->priceScaleCode = priceScaleCode;
    bookLevel->sequence = eventSequence;
    bookLevel->shares = shares;
    bookLevel->timestamp = exchTime;
    bookLevel->updateTS = recvTime;
    book.emplace(pos, std::move(bookLevel));
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::modify_price_level(Book &book, int64_t price, uint8_t priceScaleCode, uint64_t shares, uint32_t orders,
                                  uint64_t exchTime, const char *callFunc) {
    bool found = false;
    auto pos = std::lower_bound(book.begin(), book.end(), price);
    if (pos != book.end() && (*pos)->price == price) {
        auto &level = *pos;
        level->exchange = exchange;
        level->lotSize = lotSize;
        level->orders = orders;
        level->priceScaleCode = priceScaleCode;
        level->sequence = eventSequence;
        level->shares = shares;
        level->timestamp = exchTime;
        level->updateTS = producerReceiveTime;
        found = true;
    }
    return found;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::modify_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                                  uint32_t orders, uint64_t exchTime, const char *callFunc) {
    if (level >= book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range." << level);
        return false;
    }

    auto pos = std::next(book.begin(), static_cast<typename decltype(book.begin())::difference_type>(level));
    if ((*pos)->price != price) {
        MIDAS_LOG_WARNING(callFunc << " price does not exist " << price);
        return false;
    }

    auto &&bookLevel = *pos;
    bookLevel->exchange = exchange;
    bookLevel->lotSize = lotSize;
    bookLevel->orders = orders;
    bookLevel->priceScaleCode = priceScaleCode;
    bookLevel->sequence = eventSequence;
    bookLevel->shares = shares;
    bookLevel->timestamp = exchTime;
    bookLevel->updateTS = producerReceiveTime;
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::fill_price_level(Book &book, std::size_t level, int64_t price, uint8_t priceScaleCode, uint64_t shares,
                                uint32_t orders, uint64_t exchTime, const char *callFunc) {
    if (level >= book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range." << level);
        return false;
    }

    auto pos = std::next(book.begin(), static_cast<typename decltype(book.begin())::difference_type>(level));
    if ((pos != book.begin() && !(*std::prev(pos) < price)) ||
        (pos != std::prev(book.end()) && !(price < *std::next(pos)))) {
        MIDAS_LOG_WARNING(callFunc << " price is out of order." << price);
        return false;
    }

    auto &&bookLevel = *pos;
    bookLevel->price = price;
    bookLevel->exchange = exchange;
    bookLevel->lotSize = lotSize;
    bookLevel->orders = orders;
    bookLevel->priceScaleCode = priceScaleCode;
    bookLevel->sequence = eventSequence;
    bookLevel->shares = shares;
    bookLevel->timestamp = exchTime;
    bookLevel->updateTS = producerReceiveTime;
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::remove_price_level(Book &book, int64_t price, const char *callFunc) {
    bool found = false;

    auto pos = std::lower_bound(book.begin(), book.end(), price);
    if (pos != book.end() && (*pos)->price == price) {
        book.erase(pos);
        found = true;
    }

    if (!found) {
        MIDAS_LOG_WARNING(callFunc << " price does not exist" << price);
    }
    return found;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::delete_price_level(Book &book, std::size_t level, const char *callFunc) {
    if (level >= book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range." << level);
        return false;
    }

    auto pos = std::next(book.begin(), static_cast<typename decltype(book.begin())::difference_type>(level));
    book.erase(pos);
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::delete_from_price_level(Book &book, std::size_t level, const char *callFunc) {
    if (level >= book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range." << level);
        return false;
    }

    book.resize(level);
    return true;
}

// Called from market data thread only
template <typename Book>
bool MdTicker::delete_through_price_level(Book &book, std::size_t level, const char *callFunc) {
    if (level == 0 || level > book.size()) {
        MIDAS_LOG_WARNING(callFunc << " level is out of range." << level);
        return false;
    }

    auto pos = (level < book.size())
                   ? std::next(book.begin(), static_cast<typename decltype(book.begin())::difference_type>(level))
                   : book.end();
    book.erase(book.begin(), pos);
    return true;
}

#endif
