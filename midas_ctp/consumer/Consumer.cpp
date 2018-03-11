#include <fcntl.h>
#include <midas/md/MdDefs.h>
#include <midas/md/MdLock.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <net/shm/SymbolData.h>
#include <boost/exception/diagnostic_information.hpp>
#include "Consumer.h"
#include "MySymbol.h"
#include "SubControlChannel.h"
#include "SubDataChannel.h"

using namespace midas;

char const* consumer_strerror(uint32_t errcode) {
    static char const* errstr[] = {"OK",
                                   "NOT_CONNECTED_TO_PRODUCER",
                                   "NOT_AUTHORIZED",
                                   "BAD_SYMBOL",
                                   "DATA_SOURCE_ERROR",
                                   "SLOW_CONSUMER",
                                   "SHM_ERROR",
                                   "CONNECTION_TIMED_OUT",
                                   "BAD_BOOK",
                                   "NOT_SUBSCRIBED",
                                   "NO_DATA",
                                   "ALREADY_SUBSCRIBED"};

    if (errcode < sizeof(errstr) / sizeof(errstr[0])) {
        return errstr[errcode];
    }
    return "UNKNOWN";
}

Consumer::Consumer(int argc, char** argv) : MidasProcessBase(argc, argv), pid(getpid()) {
    init_admin();
    startTime = midas::ntime();
    session = startTime;

    subscriptionFlags |= FLAG_SEND_BOOK_REFRESHED;
    subscriptionFlags |= FLAG_SEND_BOOK_CHANGED;
    subscriptionFlags |= FLAG_SEND_TRADING_ACTION;
    subscriptionFlags |= FLAG_SEND_IMBALANCE;
    subscriptionFlags |= FLAG_SEND_DATA_HEARTBEAT;
}

Consumer::~Consumer() {}

void Consumer::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        THROW_MIDAS_EXCEPTION("failed to configure");
    }

    dataChannel.reset(new SubDataChannel("ctp", this));

    controlChannel.reset(new SubControlChannel("ctp", this));
    controlChannel->start();
    controlChannel->wait_until_connected();  // Block here until connected or timed out

    dataChannel->start();

    clientWorker_ = std::thread([this] { client_logic(); });
}

void Consumer::client_logic() {
    uint32_t ret;
    if ((ret = alloc_book(pSymbol->exchange, &pSymbol->book, true)) == CONSUMER_OK) {
        if ((ret = subscribe(&(pSymbol->sub), pSymbol->name, pSymbol->exchange, pSymbol.get(), false)) == CONSUMER_OK) {
            sleep(2);
            for (;;) {
                if (failedSubscription) {  // This is asynchronous so need to be checked again here
                    break;
                }
                int n = fileno(stdin);
                fd_set fdSet;
                FD_ZERO(&fdSet);
                FD_SET(n, &fdSet);
                struct timeval tVal {
                    0, 3000
                };
                if (select(n + 1, &fdSet, nullptr, nullptr, (poll ? &tVal : nullptr)) == 1 && FD_ISSET(n, &fdSet)) {
                    break;
                } else {
                    data_poll();
                    if ((ret = pSymbol->snap()) != CONSUMER_OK) {
                        MIDAS_LOG_ERROR("snap error " << consumer_strerror(ret));
                    }
                }
            }

            if ((ret = unsubscribe(&(pSymbol->sub))) != CONSUMER_OK) {
                MIDAS_LOG_ERROR("Failed to submit unsubscribe request for " << pSymbol->name
                                                                            << " error: " << consumer_strerror(ret));
            }
        } else {
            MIDAS_LOG_ERROR("subscribe failed, "
                            << " error: " << consumer_strerror(ret));
        }
    } else {
        MIDAS_LOG_ERROR("alloc_book failed, "
                        << " error: " << consumer_strerror(ret));
    }
}

void Consumer::app_stop() {
    MIDAS_LOG_INFO("Destroying consumer");

    if (dataChannel) {  // Stop reading from ring buffers
        dataChannel->stop();
    }

    if (controlChannel) {  // Send disconnect message to producer and stop the control channel
        controlChannel->stop();
    }

    if (clientWorker_.joinable()) {
        clientWorker_.join();
    }
}

bool Consumer::configure() {
    {
        const string key{"ctp"};
        userName = midas::get_cfg_value<std::string>(key, "user_name");
        MIDAS_LOG_INFO("userName=" << userName);
        password = midas::get_cfg_value<std::string>(key, "password");

        shmName = midas::get_cfg_value<std::string>(key, "name");
        MIDAS_LOG_INFO("name=" << shmName);

        clientId = midas::get_cfg_value<uint8_t>(key, "client_id");
        MIDAS_LOG_INFO("clientId=" << static_cast<uint32_t>(clientId));
        // Client ID must be usable as a bit index in shared memory read locks
        if (clientId > (MdLockMetaOffset - 1)) {
            std::ostringstream oss;
            oss << "Client ID out of range 0.." << (MdLockMetaOffset - 1);
            throw std::runtime_error(oss.str());
        }
        max_opt_snap_attempts = midas::get_cfg_value<uint32_t>(key, "max_opt_snap_attempts", 20);
        MIDAS_LOG_INFO("max_opt_snap_attempts=" << max_opt_snap_attempts);
        max_lock_snap_attempts = midas::get_cfg_value<uint32_t>(key, "max_lock_snap_attempts", 5);
        MIDAS_LOG_INFO("max_lock_snap_attempts=" << max_lock_snap_attempts);

        reader_lock_timeout = midas::get_cfg_value<uint32_t>(key, "reader_lock_timeout", 5000);
        MIDAS_LOG_INFO("reader_lock_timeout=" << reader_lock_timeout);
    }

    {
        const string key{"ctp.subscribe"};
        string symbol = midas::get_cfg_value<std::string>(key, "symbol");
        uint16_t exchangeCode = midas::get_cfg_value<uint16_t>(key, "exchange_code");
        pSymbol.reset(new MySymbol(symbol, exchangeCode));
    }
    return true;
}

void Consumer::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&Consumer::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Consumer::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << setw(24) << "book changed: " << setw(24) << bookChangedEventCount << setw(24)
        << "book refreshed: " << setw(24) << bookRefreshedEventCount << "\n";
    return oss.str();
}

void Consumer::callback_subscribe_response(char const* symbol, uint16_t exch, uint8_t statusSubscription,
                                           void* /*userData*/) {
    MIDAS_LOG_INFO("callback_subscribe_response status: " << statusSubscription << " " << symbol);
    if (statusSubscription != CONSUMER_OK) {
        failedSubscription = true;
    }
}

void Consumer::callback_book_changed(char const* symbol, uint16_t exch, BookChanged const* /*bkchngd*/,
                                     void* userData) {
    ++bookChangedEventCount;
    MySymbol* pSymbol = (MySymbol*)userData;
    pSymbol->snap();
}

void Consumer::callback_book_refreshed(char const* symbol, uint16_t exch, void* userData) {
    ++bookRefreshedEventCount;
    MySymbol* pSymbol = (MySymbol*)userData;
    pSymbol->snap();
}

bool Consumer::ready() const { return controlChannel->connected; }

void Consumer::data_poll() { dataChannel->loop_once(); }

void Consumer::on_control_channel_connected() { MIDAS_LOG_INFO("Control channel is now connected"); }

void Consumer::on_control_channel_disconnected() { MIDAS_LOG_INFO("Control channel is now disconnected"); }

void Consumer::on_control_channel_established() {
    MIDAS_LOG_INFO("Control channel is now established");
    resubscribe();
}

uint8_t Consumer::subscribe(Subscription** ppSub, std::string const& mdTick, uint16_t mdExch, void* userData,
                            bool duplicateOK) {
    MIDAS_LOG_INFO("Consumer subscribing to " << mdTick << "." << mdExch << ", " << (uint64_t)userData << ", "
                                              << duplicateOK);

    uint8_t rc;
    if (mdTick.length() >= sizeof(CtrlSubscribe::symbol))  // symbol too long!
    {
        return CONSUMER_BAD_SYMBOL;
    }

    std::string key(mdTick);

    std::lock_guard<std::recursive_mutex> g(subscriptionLock);

    // get book cache for the specified exchange
    auto iter = bookCaches.find(mdExch);
    if (bookCaches.end() == iter) {
        MIDAS_LOG_WARNING("Cannot subscribe to: " << mdTick << "." << mdExch << ": NO bookCache");
        return CONSUMER_BAD_BOOK;
    }
    auto& bookCache = iter->second;

    auto where = subscriptionByName.find(key);
    if (where != subscriptionByName.end()) {  // Existing subscription
        std::shared_ptr<SubTicker>& ticker = where->second;
        if (duplicateOK) {  // Same shmName is allowed to be subscribed multiple times
            *ppSub = ticker->subscription_add(userData ? userData : globalUserData);
            MIDAS_LOG_INFO("Duplicated subscription to " << mdTick << "." << mdExch << "|"
                                                         << (uint64_t)(userData ? userData : globalUserData));
            rc = CONSUMER_OK;
        } else {  // Subscription must always be unique
            *ppSub = nullptr;
            MIDAS_LOG_INFO("Already subscribed to " << mdTick << "." << mdExch);
            rc = CONSUMER_ALREADY_SUBSCRIBED;
        }
    } else {  // Brand new subscription
        auto p = subscriptionByName.insert(std::make_pair(
            key, std::shared_ptr<SubTicker>(new SubTicker(*this, mdTick, mdExch, bookCache->postMetadataAddress(),
                                                          bookCache->bytesPerProduct(), bookCache->bytesOffsetBid(),
                                                          bookCache->bytesOffsetAsk()))));
        auto& mdt = p.first->second;
        *ppSub = mdt->subscription_add(userData ? userData : globalUserData);
        mdt->state = SubTicker::State::pending;
        if ((rc = controlChannel->send_subscribe(mdTick, mdExch)) == CONSUMER_OK) {
            MIDAS_LOG_INFO("Queued subscribe request for " << mdTick << "." << mdExch << " | "
                                                           << (uint64_t)(userData ? userData : globalUserData));
        } else {
            MIDAS_LOG_WARNING("Failed to queue subscribe request for "
                              << mdTick << "." << mdExch << "|" << (uint64_t)(userData ? userData : globalUserData));
        }
    }
    return rc;
}

void Consumer::resubscribe() {
    std::lock_guard<std::recursive_mutex> g(subscriptionLock);

    for (auto it = subscriptionByName.begin(); it != subscriptionByName.end(); ++it) {
        MIDAS_LOG_INFO("Resubscribing to " << it->second->mdTick << "." << it->second->mdExch);
        if (controlChannel->send_subscribe(it->second->mdTick, it->second->mdExch) == CONSUMER_OK) {
            MIDAS_LOG_INFO("Posted resubscribe request for " << it->second->mdTick << "." << it->second->mdExch
                                                             << " to control channel");
        } else {
            MIDAS_LOG_WARNING("Failed to queue resubscribe request for " << it->second->mdTick << "."
                                                                         << it->second->mdExch);
        }
    }
}

uint8_t Consumer::unsubscribe(Subscription** ppsub) {
    uint8_t rc = CONSUMER_NOT_SUBSCRIBED;

    std::string mdTick = (*ppsub)->md_tick();
    uint16_t mdExch = (*ppsub)->exchange();
    uint8_t mdStrm = (*ppsub)->stream_id();
    uint16_t mdLocate = (*ppsub)->locate();

    std::string key(mdTick);

    {
        std::lock_guard<std::recursive_mutex> g(subscriptionLock);

        auto where = subscriptionByName.find(key);  // Look up the ticker first
        if (where != subscriptionByName.end()) {
            std::shared_ptr<SubTicker>& ticker = where->second;
            if (ticker->subscription_delete(*ppsub))  // Now delete the matching subscription
            {
                *ppsub = nullptr;
                if (ticker->subscription_count() == 0)  // Ticker can be removed
                {
                    if ((rc = controlChannel->send_unsubscribe(mdTick, mdExch)) == CONSUMER_OK) {
                        subscriptionByName.erase(where);
                        subscriptionByCode[mdStrm][mdLocate].reset();
                        MIDAS_LOG_INFO("Queued unsubscribe request for " << mdTick << "." << mdExch);
                    } else {
                        MIDAS_LOG_WARNING("Failed to queue unsubscribe request for " << mdTick << "." << mdExch);
                    }
                }
            }
        } else {
            MIDAS_LOG_WARNING("No such subscription " << mdTick << "." << mdExch);
            rc = CONSUMER_NOT_SUBSCRIBED;
        }
    }
    return rc;
}

uint8_t Consumer::alloc_book(MdBook* book, bool alloc) {
    uint8_t rc = CONSUMER_BAD_BOOK;

    if (book->__thunk && memcmp(book->__thunk, BOOK_WATERMARK, 8) == 0) {
        uint16_t levels = 0;
        for (BookMetadata* p = (BookMetadata*)(book->__thunk); p->exchange != ExchangeNone; ++p) {
            levels = static_cast<uint16_t>(levels + (book->bookType == MdBook::BookType::price ? p->exchangeDepth : 1));
        }
        book->numBidLevels = levels;
        book->numAskLevels = levels;
        if (alloc) {
            book->bidLevels = new BidBookLevel[book->numBidLevels];
            book->askLevels = new AskBookLevel[book->numAskLevels];
        } else {
            book->myAlloc = 0;
        }
        rc = CONSUMER_OK;
    }
    return rc;
}

uint8_t Consumer::alloc_book(uint16_t mdExch, MdBook* book, bool alloc) {
    uint8_t rc = CONSUMER_BAD_BOOK;

    auto iter = bookCaches.find(mdExch);
    if (bookCaches.end() == iter) {
        return rc;
    }
    auto& bookCache = iter->second;

    if (book && bookCache) {
        book->__thunk = (BookMetadata*)bookCache->startAddress();
        book->bookType = MdBook::BookType::price;
        rc = alloc_book(book, alloc);
    }
    return rc;
}

void Consumer::on_trading_action(Header* /*pHeader*/, DataTradingAction* ta) {
    std::string key(ta->symbol);
    std::lock_guard<std::recursive_mutex> g(subscriptionLock);
    auto where = subscriptionByName.find(key);
    if (where != subscriptionByName.end()) {
        std::shared_ptr<SubTicker>& ticker = where->second;
        ticker->on_trading_action(ta->symbol, ta->exchange, &(ta->tradingAction));
    }
}

void Consumer::on_book_refreshed(Header* pHeader, DataBookRefreshed* refreshed) {
    std::string key(refreshed->symbol);
    std::lock_guard<std::recursive_mutex> g(subscriptionLock);
    auto where = subscriptionByName.find(key);
    if (where != subscriptionByName.end()) {
        std::shared_ptr<SubTicker>& ticker = where->second;
        ticker->streamId = pHeader->streamId;
        ticker->locate(refreshed->locate);
        if (subscriptionByCode[pHeader->streamId][refreshed->locate].get() != ticker.get()) {
            subscriptionByCode[pHeader->streamId][refreshed->locate] = ticker;
        }
        ticker->on_book_refreshed(refreshed->symbol, refreshed->exchange);
    }
}

void Consumer::on_book_changed(Header* /*pHeader*/, DataBookChanged* changed) {
    std::string key(changed->symbol);
    std::lock_guard<std::recursive_mutex> g(subscriptionLock);
    auto where = subscriptionByName.find(key);
    if (where != subscriptionByName.end()) {
        std::shared_ptr<SubTicker>& ticker = where->second;
        ticker->on_book_changed(changed->symbol, changed->exchange, &(changed->bookChanged));
    }
}

void Consumer::on_data_heartbeat(Header* /*pHeader*/, DataHeartbeat* dataHeartbeat) {
    //    MIDAS_LOG_INFO("on_data_heartbeat called");
}

void Consumer::on_connect_response(Header* pHeader, CtrlConnectResponse* response, bool* isConnected) {
    MIDAS_LOG_INFO("Connect status " << response->status);

    if (response->status == CONNECT_STATUS_OK) {
        session = pHeader->session;  // Note the session
        MIDAS_LOG_INFO("Publisher Session ID " << session);

        bool result = true;
        switch (response->shmType) {
            case SHM_TYPE_EVENT_QUEUE: {
                dataChannel->create_queue(response->shmPath);
                break;
            }
            case SHM_TYPE_BOOK_CACHE: {
                auto iter = bookCaches.find(response->exchange);
                if (bookCaches.end() == iter) {
                    bookCaches.emplace(response->exchange,
                                       std::make_unique<ShmCacheConsumer>(response->shmPath, response->shmSize));
                } else if (iter->second->name() != response->shmPath) {
                    MIDAS_LOG_ERROR("Existing bookCache for exchange: "
                                    << response->exchange << " found, but shmName mismatch; "
                                    << "expected: " << response->shmPath << ", actual: " << iter->second->name());
                    result = false;
                }
                break;
            }
            default:
                break;  // can't be here
        }
        connected = result;
    } else {
        if (response->status == CONNECT_STATUS_ALREADY_CONNECTED) {
            MIDAS_LOG_INFO("Client " << response->cid << " is already connected");
        } else if (response->status != CONNECT_STATUS_OK) {
            MIDAS_LOG_INFO("Client " << response->cid << " failed to connect");
        }
    }
    *isConnected = connected;
}

void Consumer::on_disconnect_response(Header* /*pHeader*/, CtrlDisconnectResponse* response) {
    MIDAS_LOG_INFO("Disconnect status " << response->status);
}

void Consumer::on_subscribe_response(Header* /*pHeader*/, CtrlSubscribeResponse* response) {
    auto const& locate = response->locate;
    auto const& exch = response->exchange;

    MIDAS_LOG_INFO("Received SubscribeResponse, status " << response->status << ", locate: " << locate
                                                         << ", symbol: " << response->symbol << ", exch: " << exch);

    bool subOk = (SUBSCRIBE_STATUS_OK == response->status);
    std::string key(response->symbol);

    std::lock_guard<std::recursive_mutex> g(subscriptionLock);

    // if the key specified above is correct, it should be found since a subscription was made
    auto where = subscriptionByName.find(key);  // Look up the ticker first
    if (where != subscriptionByName.end()) {
        if (subOk) {  // only set the locator code if found & successful
            where->second->locate(response->locate);
        }
        // pass the subscribe response back to the application
        where->second->on_subscribe_response(response->symbol, exch, subOk ? CONSUMER_OK : CONSUMER_BAD_SYMBOL);
    }
}

void Consumer::on_unsubscribe_response(Header* /*pHeader*/, CtrlUnsubscribeResponse* response) {
    auto const& locate = response->locate;
    auto const& exch = response->exchange;
    MIDAS_LOG_INFO("Received UnsubscribeResponse, status " << response->status << ", locate: " << locate
                                                           << ", symbol: " << response->symbol << ", exch: " << exch);
}
