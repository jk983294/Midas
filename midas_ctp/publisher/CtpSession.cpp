#include <algorithm>
#include <iterator>
#include <sstream>
#include "ConsumerProxy.h"
#include "CtpSession.h"
#include "Publisher.h"

using namespace std;
using namespace midas;

static constexpr int ctpDepth = 5;
MdExchange CtpSession::ctpExchange{ExchangeCtpAll, ctpDepth, bytes_per_product(ctpDepth), 0,
                                   bytes_offset_ask(ctpDepth)};

MdExchange CtpSession::participant_exchanges() { return ctpExchange; }

CtpSession::CtpSession(Publisher const& publisher, uint8_t queueIndex) : DataSource(publisher, queueIndex) {
    marketManager = make_shared<MarketManager<CtpSessionConsumer>>(std::make_shared<CtpSessionConsumer>(subscriptions));
    marketManager->start();
}

CtpSession::~CtpSession() {}

// Called from ControlChannel thread
void CtpSession::subscribe(std::string const& symbol, std::shared_ptr<ConsumerProxy> const& consumerProxy) {
    std::string s;
    std::vector<__data*> allSymbolData;
    uint16_t locator = 0;
    auto bookCache = publisher.book_cache(ExchangeCtpAll);
    if (!bookCache->symbolData->locate(symbol, locator, true) ||
        !bookCache->symbolData->locate(locator, allSymbolData)) {
        MIDAS_LOG_INFO("failed to locator " << symbol << " in shm");
        consumerProxy->send_subscribe_response(SUBSCRIBE_STATUS_BAD_SYMBOL, UINT16_MAX, symbol.c_str(), ExchangeCtpAll);
        return;
    }

    consumerProxy->send_subscribe_response(SUBSCRIBE_STATUS_OK, locator, symbol.c_str(), ExchangeCtpAll);

    for (auto&& mySymbolData : allSymbolData) {
        std::string midasSymbol(get_symbol(mySymbolData));
        bool found = false;
        auto itr = subscriptions.find(midasSymbol);
        if (itr != subscriptions.end()) {
            auto& ticker = itr->second;
            if (ticker) {  // The subscription for symbol is actually live so just add this subscriber to it
                consumerProxy->send_subscribe_response(SUBSCRIBE_STATUS_OK, ticker->locate, symbol.c_str(),
                                                       ExchangeCtpAll);
                ticker->add_subscriber(consumerProxy, true);
                refreshNeeded.put(ticker);
                found = true;
            } else {
                subscriptions.insert(std::make_pair(midasSymbol, std::shared_ptr<MdTicker>()));
            }
        } else {
            subscriptions.insert(std::make_pair(midasSymbol, std::shared_ptr<MdTicker>()));
        }

        if (!found) {  // Brand new subscription or reset
            auto& ticker = subscriptions[midasSymbol];
            ticker.reset(new MdTicker(*this, midasSymbol, consumerProxy->publisher->book_cache_addr(ExchangeCtpAll),
                                      CtpSession::ctpExchange));
            ticker->add_subscriber(consumerProxy);
            ticker->set_locate(locator);
            ticker->state = MdTicker::State::active;
        }
    }

    marketManager->mdSpi->subscribe(symbol);
}

// Called from ControlChannel thread
void CtpSession::unsubscribe(std::string const& symbol, std::shared_ptr<ConsumerProxy> const& consumerProxy) {
    auto mdkeySubIt = subscriptions.find(symbol);
    if (mdkeySubIt != subscriptions.end()) {
        std::shared_ptr<MdTicker>& ticker = mdkeySubIt->second;
        if (!ticker) return;
        if (ticker->is_subscriber(consumerProxy)) {  // is the symbol subscribed by this consumer proxy
            ticker->remove_subscriber(consumerProxy);
            consumerProxy->send_unsubscribe_response(*ticker);
            if (!ticker->has_subscriber()) {  // watch count dropped to 0
                // CTP subscription is never unsubscribed by default
                if (canUnsubscribe) {
                    ticker->exchange = ExchangeNone;
                    ticker.reset();
                } else {
                    /* the ticker object is never destroyed nor removed from the subscription map */
                }
            }
        }
    }
}

// Must be called from ControlChannel thread when test_mode == true
void CtpSession::clear_all_market_data() {
    for (auto&& p1 : subscriptions) {
        if (p1.second) {
            p1.second->clear_bid();
            p1.second->clear_ask();
        }
    }
}

// Must be called from ControlChannel thread
void CtpSession::clear_all_book_cache() {
    for (auto&& p1 : subscriptions) {
        if (p1.second) {
            clear_book_cache(&p1.second);
        }
    }
}

// Called from ControlChannel thread
std::size_t CtpSession::unregister_consumer_hook(std::shared_ptr<ConsumerProxy> const& consumerProxy) {
    uint32_t nWatch = 0;
    for (auto&& p1 : subscriptions) {
        if (p1.second && p1.second->is_subscriber(consumerProxy)) {
            unsubscribe(p1.first, consumerProxy);
            nWatch += 1;
        }
    }
    return nWatch;
}

// Called from both ControlChannel thread and market data threads
bool CtpSession::locate(std::string const& midasSymbol, uint16_t& locator) {
    auto bookCache = publisher.book_cache(ExchangeCtpAll);
    return bookCache ? bookCache->symbolData->locate(midasSymbol, locator) : false;
}

// Called from ControlChannel thread
void CtpSession::on_call_house_keeping() {}

//
// called from market data thread
// There is one command queue (SPSC) for each Exegy market data thread through which MDControlChannel thread
// can post commands to be executed in market data thread context
//
bool CtpSession::poll_command_queue() {
    uint8_t* ptr = nullptr;
    Header* pHeader = nullptr;
    while (circularBuffer.get_read_pointer(ptr, sizeof(Header)) == sizeof(Header)) {
        pHeader = reinterpret_cast<Header*>(ptr);
        uint32_t size = pHeader->size;
        const uint32_t count = pHeader->count;
        circularBuffer.commit_read(sizeof(Header));
        uint8_t* pData = nullptr;
        if (size > 0 && circularBuffer.get_read_pointer(pData, size) == size) {
            for (uint32_t c = 0; c < count; ++c) {
                switch (*pData) {
                    case ADMIN_BOOK_CLEAR_TYPE: {
                        std::shared_ptr<MdTicker>* tickerPtr =
                            reinterpret_cast<std::shared_ptr<MdTicker>*>(((AdminBookClear*)pData)->ticker);
                        std::shared_ptr<MdTicker> ticker;
                        if (tickerPtr) ticker = *tickerPtr;  // make a local shared copy to avoid destruction;
                        if (ticker) {
                            MIDAS_LOG_INFO("Clear Bid/Ask of " << ticker->mdTick);
                            ticker->clear_bid();
                            ticker->clear_ask();
                        }
                        break;
                    }
                    default:
                        break;
                }
                pData += 1;             // advance by 1 byte to size field
                pData += (*pData - 1);  // advance to next message
            }
            circularBuffer.commit_read(size);
        }
    }

    return true;
}
