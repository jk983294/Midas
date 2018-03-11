#include "ConsumerProxy.h"
#include "MdTicker.h"

using namespace std;
using namespace midas;

MdTicker::MdTicker(DataSource& dataSource_, std::string const& mdTick_, uint8_t* addrBookCache_,
                   MdExchange const& exchangeInfo)
    : mdTick(mdTick_),
      dataSource(dataSource_),
      exchange(exchangeInfo.exchange),
      bytesPerProduct(exchangeInfo.bytesPerProduct),
      shmBookAllProducts(addrBookCache_),
      shmBookBidLevels(exchangeInfo.exchangeDepth),
      offsetBytesBid(exchangeInfo.offsetBytesBid),
      shmBookAskLevels(shmBookBidLevels),
      offsetBytesAsk(exchangeInfo.offsetBytesAsk) {
    /*
     * empty book is now empty: it no longer contains 1 dummy level a "blank" book
     */
}

MdTicker::~MdTicker() {}

void MdTicker::add_subscriber(std::shared_ptr<ConsumerProxy> const& consumer, bool asyncRefresh) {
    {
        const auto cBeg = consumers.cbegin();
        const auto cEnd = consumers.cend();
        if (std::find(cBeg, cEnd, consumer) == cEnd) {
            const std::lock_guard<std::mutex> g(tickerLock);
            consumers.emplace_back(consumer);
        }
    }

    if (asyncRefresh) {
        const auto cBeg = std::begin(consumersPendingRefresh);
        const auto cEnd = std::end(consumersPendingRefresh);
        if (std::find(cBeg, cEnd, consumer) == cEnd) {
            const std::lock_guard<std::mutex> g(tickerLock);
            consumersPendingRefresh.emplace_back(consumer);
        }
    }
}

bool MdTicker::remove_subscriber(std::shared_ptr<ConsumerProxy> const& consumer) {
    bool found = false;
    {
        const auto cBeg = std::begin(consumers);
        const auto cEnd = std::end(consumers);
        const auto result = std::find(cBeg, cEnd, consumer);
        if (result != cEnd) {
            // remove lock if any
            if (shmLockBid) {
                MdLock::_unlock_reader(shmLockBid, static_cast<uint8_t>(consumer->clientId));
            }
            if (shmLockAsk) {
                MdLock::_unlock_reader(shmLockAsk, static_cast<uint8_t>(consumer->clientId));
            }

            const std::lock_guard<std::mutex> g(tickerLock);
            consumers.erase(result);
            found = true;
        }
    }

    {
        const auto cBeg = std::begin(consumersPendingRefresh);
        const auto cEnd = std::end(consumersPendingRefresh);
        const auto result = std::find(cBeg, cEnd, consumer);
        if (result != cEnd) {
            const std::lock_guard<std::mutex> g(tickerLock);
            consumersPendingRefresh.erase(result);
        }
    }
    return found;
}

bool MdTicker::is_subscriber(std::shared_ptr<ConsumerProxy> const& consumer) {
    const auto cBeg = std::begin(consumers);
    const auto cEnd = std::end(consumers);
    return (std::find(cBeg, cEnd, consumer) != cEnd);
}

bool MdTicker::has_subscriber() const { return consumers.size() > 0; }

/*
 * Called from md processing thread via callback with lock (tickerLock) held
 */
void MdTicker::send_pending_refresh() {
    if (state != MdTicker::State::badSymbol && !consumersPendingRefresh.empty()) {
        for (auto&& consumer : consumersPendingRefresh) {
            consumer->send_book_refreshed(*this);
        }
        consumersPendingRefresh.clear();
    }
}

/*
 * Called from md processing thread via callback with lock (tickerLock) held
 */
void MdTicker::cancel_pending_refresh() {
    if (!consumersPendingRefresh.empty()) {
        consumersPendingRefresh.clear();
    }
}

/*
 * Called from md processing thread via callback with lock (tickerLock) held
 */
void MdTicker::on_subscribe(State state_) {
    const std::lock_guard<std::mutex> g(tickerLock);
    state = state_;
    for (auto&& consumer : consumers) {
        consumer->send_subscribe_response(*this);
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_refresh_L1() {
    init_offsets();
    {
        MdLock::VersionGuard guard{shmLockBid};
        ((BidBookLevel*)shmBookBid)[0] = bookBid.size() ? *(bookBid[0]) : blank_bid_level();
    }
    {
        MdLock::VersionGuard guard{shmLockAsk};
        ((AskBookLevel*)shmBookAsk)[0] = bookAsk.size() ? *(bookAsk[0]) : blank_ask_level();
    }

    {
        // Send refresh event
        const std::lock_guard<std::mutex> g(tickerLock);
        for (auto&& consumer : consumers) {
            consumer->send_book_refreshed(*this);
        }
        cancel_pending_refresh(); /* cancel all pending refreshes */
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_quote_L1(BookChanged::ChangedSide side) {
    {
        bookChangedSide = side;
        init_offsets();
        {
            MdLock::VersionGuard guard{shmLockBid};
            ((BidBookLevel*)shmBookBid)[0] = bookBid.size() ? *(bookBid[0]) : blank_bid_level();
        }
        {
            MdLock::VersionGuard guard{shmLockAsk};
            ((AskBookLevel*)shmBookAsk)[0] = bookAsk.size() ? *(bookAsk[0]) : blank_ask_level();
        }
    }

    {
        // Send change event
        const std::lock_guard<std::mutex> g(tickerLock);
        send_pending_refresh();

        for (auto&& consumer : consumers) {
            consumer->send_book_changed(*this);
        }
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_trading_action(TradingAction::TradeStatus trdst, uint16_t exch) {
    const std::lock_guard<std::mutex> g(tickerLock);

    send_pending_refresh();

    if (trdst != tradingAction.state) {
        tradingAction.state = trdst;
        for (auto&& consumer : consumers) {
            consumer->send_trading_action(*this, exch);
        }
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_refresh_bid() {
    {
        init_offsets();
        const std::size_t n = bookBid.size();
        const auto blankBid = blank_bid_level();
        MdLock::VersionGuard guard{shmLockBid};
        // copy book from ticker to shm
        for (std::size_t u = 0; u < shmBookBidLevels && u < n; ++u) {
            ((BidBookLevel*)shmBookBid)[u] = *(bookBid[u]);
        }
        // pad shm with dummy levels up to count
        for (std::size_t u = n; u < shmBookBidLevels; ++u) {
            ((BidBookLevel*)shmBookBid)[u] = blankBid;
        }
    }

    {
        // Send refresh event
        const std::lock_guard<std::mutex> g(tickerLock);
        for (auto&& consumer : consumers) {
            consumer->send_book_refreshed(*this);
        }
        cancel_pending_refresh(); /* cancel all pending refreshes */
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_refresh_ask() {
    {
        init_offsets();

        const std::size_t n = bookAsk.size();
        const auto blankAsk = blank_ask_level();
        MdLock::VersionGuard guard{shmLockAsk};
        for (std::size_t u = 0; u < shmBookAskLevels && u < n; ++u) {
            ((AskBookLevel*)shmBookAsk)[u] = *(bookAsk[u]);
        }
        for (std::size_t u = n; u < shmBookAskLevels; ++u) {
            ((AskBookLevel*)shmBookAsk)[u] = blankAsk;
        }
    }

    {
        // Send refresh event
        const std::lock_guard<std::mutex> g(tickerLock);

        for (auto&& consumer : consumers) {
            consumer->send_book_refreshed(*this);
        }

        cancel_pending_refresh(); /* cancel all pending refreshes */
    }
}

/*
 * Called from md processing thread
 */
void MdTicker::process_book_update(BookChanged::ChangedSide side, std::size_t numOrigBidLevels,
                                   std::size_t numOrigAskLevels) {
    bookChangedSide = side;

    {
        init_offsets();

        if (BookChanged::ChangedSide::both == side || BookChanged::ChangedSide::bid == side) {
            const std::size_t n = bookBid.size();

            MdLock::VersionGuard guard{shmLockBid};
            if (n < numOrigBidLevels) {  // deletion has occurred, clear levels if necessary
                for (std::size_t index = n; index < shmBookBidLevels && index < numOrigBidLevels; ++index) {
                    BidBookLevel& t = ((BidBookLevel*)shmBookBid)[index];
                    clearBidLevel(&t);
                }
            }

            uint64_t totalShares = 0;
            for (uint8_t u = 0; u < shmBookBidLevels && u < n; ++u)  // copy up to shmBookBidLevels
            {
                BidBookLevel& s = *(bookBid[u]);
                BidBookLevel& t = ((BidBookLevel*)shmBookBid)[u];
                t = s;
                totalShares += s.shares;
            }
        }

        if (BookChanged::ChangedSide::both == side || BookChanged::ChangedSide::ask == side) {
            const std::size_t n = bookAsk.size();

            MdLock::VersionGuard guard{shmLockAsk};
            if (n < numOrigAskLevels) {  // deletion has occurred
                for (std::size_t index = n; index < shmBookAskLevels && index < numOrigAskLevels; ++index) {
                    AskBookLevel& t = ((AskBookLevel*)shmBookAsk)[index];
                    clearAskLevel(&t);
                }
            }

            uint64_t totalShares = 0;
            for (uint8_t u = 0; u < shmBookAskLevels && u < n; ++u) {  // copy up to shmBookAskLevels
                AskBookLevel& s = *(bookAsk[u]);
                AskBookLevel& t = ((AskBookLevel*)shmBookAsk)[u];
                t = s;
                totalShares += s.shares;
            }
        }
    }

    {
        // Send change event
        const std::lock_guard<std::mutex> g(tickerLock);
        for (auto&& consumer : consumers) {
            consumer->send_book_changed(*this);
        }
    }
}

void MdTicker::init_offsets() {
    if (shmBookBid == nullptr) {
        shmBookBid = shmBookAllProducts + locate * bytesPerProduct + offsetBytesBid;
        shmLockBid = reinterpret_cast<uint64_t*>(shmBookAllProducts         /* where book cache begins */
                                                 + locate * bytesPerProduct /*where my book cache begins */
                                                 + bid_lock_offset(bytesPerProduct));
        *(shmLockBid + 1) = 0;

        BidBookLevel dummyBid;
        dummyBid.exchange = exchange;
        dummyBid.lotSize = lotSize;
        BidBookLevel* bl = (BidBookLevel*)shmBookBid;
        for (uint8_t d = 0; d < shmBookBidLevels; ++d) {
            *bl = dummyBid;
            ++bl;
        }
    }

    if (shmBookAsk == nullptr) {
        shmBookAsk = shmBookAllProducts + locate * bytesPerProduct + offsetBytesAsk;
        shmLockAsk = reinterpret_cast<uint64_t*>(shmBookAllProducts         /* where book cache begins */
                                                 + locate * bytesPerProduct /*where my book cache begins */
                                                 + ask_lock_offset(bytesPerProduct));
        *(shmLockAsk + 1) = 0;

        AskBookLevel dummyAsk;
        dummyAsk.exchange = exchange;
        dummyAsk.lotSize = lotSize;
        AskBookLevel* al = (AskBookLevel*)shmBookAsk;
        for (uint8_t d = 0; d < shmBookAskLevels; ++d) {
            *al = dummyAsk;
            ++al;
        }
    }
}

void MdTicker::clear_bid() {  // Called by market data thread only
    bookBid.clear();
    init_offsets();
    BidBookLevel dummyBid;
    dummyBid.exchange = exchange;
    dummyBid.lotSize = lotSize;
    MdLock::VersionGuard guard{shmLockBid};
    for (uint8_t u = 0; u < shmBookBidLevels; ++u) {
        ((BidBookLevel*)shmBookBid)[u] = dummyBid;
    }
}

void MdTicker::clear_ask() {  // Called by market data thread only

    bookAsk.clear();
    init_offsets();
    AskBookLevel dummyAsk;
    dummyAsk.exchange = exchange;
    dummyAsk.lotSize = lotSize;
    MdLock::VersionGuard guard{shmLockAsk};
    for (uint8_t u = 0; u < shmBookAskLevels; ++u) {
        ((AskBookLevel*)shmBookAsk)[u] = dummyAsk;
    }
}

midas::BidBookLevel MdTicker::blank_bid_level() const {
    midas::BidBookLevel blankBid;
    blankBid.exchange = exchange;
    blankBid.lotSize = lotSize;
    return blankBid;
}

midas::AskBookLevel MdTicker::blank_ask_level() const {
    midas::AskBookLevel blankAsk;
    blankAsk.exchange = exchange;
    blankAsk.lotSize = lotSize;
    return blankAsk;
}

void MdTicker::set_locate(uint16_t code) {
    locate = code;
    init_offsets();
}

void MdTicker::update_refresh_times(uint64_t srcRecvTime, uint64_t srcXmitTime, uint64_t recvTime) {
    // part of old onRefreshL1
    srcReceiveTime = srcRecvTime;
    srcTransmitTime = srcXmitTime;
    producerReceiveTime = recvTime;
}
