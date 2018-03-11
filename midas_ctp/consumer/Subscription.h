#ifndef MIDAS_SUBSCRIPTION_H
#define MIDAS_SUBSCRIPTION_H

#include <midas/md/MdBook.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <x86intrin.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct Consumer;      // Forward declaration
struct Subscription;  // Forward declaration

struct SubTicker {
    enum class State : uint8_t {
        empty,    // Ticker created but not used yet
        pending,  // Awaiting response from producer
        active    // Subscribed
    };

    SubTicker(Consumer& consumer_, std::string const& mdTick_, uint16_t mdExch_, uint8_t* l2AddrBookCache_,
              uint16_t l2BytesPerProduct_, uint16_t l2OffsetBytesBid_, uint16_t l2OffsetBytesAsk_);

    ~SubTicker() = default;

    SubTicker(SubTicker const&) = delete;

    SubTicker& operator=(SubTicker const&) = delete;

    char const* md_tick() const { return mdTick.c_str(); }

    void locate(uint16_t code);

    Subscription* subscription_add(void* userData);

    bool subscription_delete(Subscription* sub);

    void on_book_refreshed(char const* symbol, uint16_t exch);

    void on_book_changed(char const* symbol, uint16_t exch, midas::BookChanged const* bkchngd);

    void on_trading_action(char const* symbol, uint16_t exch, midas::TradingAction const* trdst);

    void on_subscribe_response(char const* symbol, uint16_t exch, uint8_t subscriptionStatus);

    std::size_t subscription_count() const {
        std::lock_guard<std::recursive_mutex> g(tickerLock);
        return subscriptions.size();
    }

    uint8_t* addr_bid_levels() const { return l2ShmBookBidMe; }

    uint8_t* addr_ask_levels() const { return l2ShmBookAskMe; }

    void init_offsets();

    uint32_t occ_snap(void* target, uint8_t* source, size_t n, uint64_t* pMeta);

    uint32_t bid_snap(midas::MdBook* book);

    uint32_t ask_snap(midas::MdBook* book);

    uint32_t snap(uint8_t side, midas::MdBook* book);

    Consumer& consumer;
    mutable std::recursive_mutex tickerLock;
    std::string mdTick;
    uint16_t mdExch{midas::ExchangeNone};
    State state{State::empty};
    uint8_t streamId{0};
    uint16_t locator{UINT16_MAX};
    using mdsubT = std::unique_ptr<Subscription>;
    std::vector<mdsubT> subscriptions;
    uint8_t* l2AddrBookCache;
    uint16_t l2BytesPerProduct;
    uint16_t l2OffsetBytesBid;
    uint16_t l2OffsetBytesAsk;
    uint64_t* l2LockBid;
    uint8_t* l2ShmBookBidMe;  // Where all the bid levels start in shared memory
    uint64_t* l2LockAsk;
    uint8_t* l2ShmBookAskMe;  // Where all the ask levels start in shared memory
};

struct Subscription {
    Subscription(SubTicker& ticker, void* userData) : ticker(ticker), userData(userData) {}

    ~Subscription() = default;

    Subscription(Subscription const&) = delete;

    Subscription& operator=(Subscription const&) = delete;

    char const* md_tick() const { return ticker.md_tick(); }

    uint16_t exchange() const { return ticker.mdExch; }

    uint8_t stream_id() const { return ticker.streamId; }

    uint16_t locate() const { return ticker.locator; }

    uint32_t snap(uint8_t side, midas::MdBook* book) { return ticker.snap(side, book); }

    SubTicker& ticker;

    void* userData{nullptr};
};

#endif
