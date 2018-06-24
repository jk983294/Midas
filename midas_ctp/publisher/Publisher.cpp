#include <fcntl.h>
#include <midas/md/MdDefs.h>
#include <midas/md/MdLock.h>
#include <net/raw/MdProtocol.h>
#include <net/raw/RawSocket.h>
#include <net/shm/SymbolData.h>
#include <boost/exception/diagnostic_information.hpp>
#include "ConsumerProxy.h"
#include "CtpSession.h"
#include "DataChannel.h"
#include "DataSource.h"
#include "PubControlChannel.h"
#include "Publisher.h"

using namespace midas;

Publisher::Publisher(int argc, char** argv) : MidasProcessBase(argc, argv) {
    init_admin();
    startTime = midas::ntime();
    session = startTime;
}

Publisher::~Publisher() { controlChannel->stop(); }

void Publisher::app_start() {
    if (!configure()) {
        MIDAS_LOG_ERROR("failed to configure");
        MIDAS_LOG_FLUSH();
        THROW_MIDAS_EXCEPTION("failed to configure");
    }
}

void Publisher::app_stop() {}

bool Publisher::configure() {
    const string key{"ctp"};
    name = midas::get_cfg_value<string>(key, "shmName");
    {
        const string cacheKey{"ctp.book_cache"};
        create_book_cache(cacheKey);
    }

    // Construct data source instance for each configured session
    std::shared_ptr<DataSource> ds(new CtpSession(*this, 0));
    dataSources.push_back(ds);

    sendBookUpdate = midas::get_cfg_value<bool>(key, "book_update_notification", false);
    houseKeepingBatch = midas::get_cfg_value<uint32_t>(key, "house_keeping_batch", 50);

    controlChannel.reset(new PubControlChannel(key, this));
    controlChannel->start();
    return true;
}

void Publisher::init_admin() {
    admin_handler().register_callback("meters", boost::bind(&Publisher::admin_meters, this, _1, _2),
                                      "display statistical information of connections", "meters");
}

string Publisher::admin_meters(const string& cmd, const TAdminCallbackArgs& args) const {
    ostringstream oss;
    oss << setw(24) << "MD login time" << setw(24) << "MD logout time" << setw(24) << "Trade login time" << setw(24)
        << "Trade logout time\n";
    return oss.str();
}

void Publisher::create_book_cache(std::string const& key) {
    std::string exchangeKey = midas::get_cfg_value<string>(key, "exchange_key");
    std::string cacheName = midas::get_cfg_value<string>(key, "shmName");
    std::string symbolData = midas::get_cfg_value<string>(key, "symbol_data_file");

    if (exchangeKey.empty() || cacheName.empty() || symbolData.empty()) {
        throw std::runtime_error("exchange_key or shmName or symbol_data_file was not specified");
    }

    auto symDat = std::make_unique<SymbolData>(symbolData, SymbolData::UserFlag::readwrite);

    uint16_t exchangeCode = midas::get_cfg_value<uint16_t>(key, "exchange_code", std::numeric_limits<uint16_t>::max());
    if (exchangeCode == std::numeric_limits<uint16_t>::max()) {
        std::ostringstream ostr;
        throw std::runtime_error("invalid exchange_key");
    }

    // verify cache does not yet exist for the exchange
    if (bookCaches.count(exchangeCode)) {
        std::ostringstream ostr;
        ostr << "book_cache already created for " << exchangeKey << ":" << exchangeCode;
        throw std::runtime_error(ostr.str());
    }

    bookCaches.emplace(exchangeCode,
                       std::make_unique<ShmCache>(key, CtpSession::participant_exchanges(), symDat.release()));
}

uint8_t* Publisher::book_cache_addr(uint16_t cacheId) const {
    auto iter = bookCaches.find(cacheId);
    return bookCaches.end() == iter ? nullptr : iter->second->address();
}

ShmCache* Publisher::book_cache(uint16_t cacheId) const {
    auto iter = bookCaches.find(cacheId);
    return bookCaches.end() == iter ? nullptr : iter->second.get();
}

// Called by the ControlChannel thread
void Publisher::register_client(int clientSock, std::string const& clientIP, uint16_t clientPort) {
    MIDAS_LOG_INFO("Registering Client IP: " << clientIP << ", Port: " << clientPort << ", Socket: " << clientSock);

    if (consumers.find(clientSock) != consumers.end()) {
        MIDAS_LOG_ERROR("There is already a registered client on sock " << clientSock);
        return;
    }

    auto x = consumers.insert(std::make_pair(
        clientSock, std::shared_ptr<ConsumerProxy>(new ConsumerProxy(clientSock, clientIP, clientPort, this,
                                                                     new DataChannel("ctp.data_channel"),
                                                                     static_cast<uint8_t>(dataSources.size())))));

    if (x.second) {
        MIDAS_LOG_INFO("Registered Client IP: " << clientIP << ", Port: " << clientPort << ", Socket: " << clientSock);
    } else {
        MIDAS_LOG_ERROR("Failed to register Client IP: " << clientIP << ", Port: " << clientPort
                                                         << ", Socket: " << clientSock);
    }
}

// Called by the ControlChannel thread
void Publisher::unregister_client(int clientSock) {
    auto cit = consumers.find(clientSock);
    if (cit != consumers.end()) {
        auto& proxy = cit->second;
        MIDAS_LOG_INFO("Unregistering client - Sock: " << clientSock << ", ID: " << proxy->clientId
                                                       << ", Key: " << proxy->shmKey());

        std::size_t nWatch = 0;
        for (auto& dataSource : dataSources) {
            nWatch += dataSource->unregister_consumer(proxy);
        }

        MIDAS_LOG_INFO("Removed " << nWatch << " tickers from watch list of subscriber " << clientSock);

        commandQueues.erase(clientSock);  // remove all pending commands
        consumers.erase(cit);
        MIDAS_LOG_INFO("Removed client " << clientSock);
    } else {
        MIDAS_LOG_ERROR("Did not find client " << clientSock);
    }
}

// Called by the ControlChannel thread
void Publisher::update_client_heartbeat(int clientSock) {
    auto cit = consumers.find(clientSock);
    if (cit != consumers.end()) {
        cit->second->update_heartbeat();
    }
}

/*
 * PubControlChannel->Publisher->ConsumerProxy::onConnect
 */
void Publisher::on_connect(int clientSock, Header* pHeader, CtrlConnect* cp) {
    MIDAS_LOG_INFO("clientSock: " << clientSock << ", streamId: " << pHeader->streamId);

    uint8_t errcode = CONNECT_STATUS_OK;

    auto cit = consumers.find(clientSock);
    if (cit == consumers.end()) {  // Has control channel registered this connection
        // This should never happen - but if it does, we don't actually have a consumer
        // proxy constructed which we can use to submit an error response.
        MIDAS_LOG_ERROR("producer failed to find a client connected to socket " << clientSock);
        uint8_t buffer[sizeof(Header) + sizeof(CtrlConnectResponse)];
        Header* hp = (Header*)buffer;
        hp->session = session;
        hp->seq = 1;
        hp->streamId = STREAM_ID_CONTROL_CHANNEL;
        hp->count = 1;
        hp->size = sizeof(CtrlConnectResponse);
        hp->transmitTimestamp = midas::ntime();
        CtrlConnectResponse* crsp = (CtrlConnectResponse*)(buffer + sizeof(Header));
        *crsp = CtrlConnectResponse();
        crsp->status = CONNECT_STATUS_UNREGISTERED_CONNECTION;
        write_socket(clientSock, buffer, sizeof(Header) + sizeof(CtrlConnectResponse));
        close_socket(clientSock);
        return;
    } else if (cp->clientId > (MdLockMetaOffset - 1)) {
        // Client ID must be usable as a bit index in shared memory read locks
        MIDAS_LOG_WARNING("Rejecting client with ID out of range: " << cp->clientId);
        errcode = CONNECT_STATUS_INVALID_ID;
    } else if (cp->clientPid <= 0) {
        MIDAS_LOG_WARNING("Rejecting client with invalid PID: " << cp->clientPid);
        errcode = CONNECT_STATUS_INVALID_ID;
    } else {
        for (auto&& p : consumers) {
            // Don't compare against any clients whose connect message has not yet been received
            if (p.second->pid() == 0) {
                continue;
            }
            // Verify if another client is already connected with same ID
            if (p.second->clientId == cp->clientId) {
                MIDAS_LOG_WARNING("Another client is already connected with this ID: " << p.second->clientId);
                errcode = CONNECT_STATUS_ALREADY_CONNECTED;
                break;
            }
            if (p.second->sockfd() == clientSock) {
                MIDAS_LOG_WARNING("A connect message has already been received from client ID " << p.second->clientId
                                                                                                << " on this socket");
                errcode = CONNECT_STATUS_ALREADY_CONNECTED;
                break;
            }
            if (p.second->shmKey() == cp->shmKey) {
                MIDAS_LOG_WARNING("Another client is already connected with this shmKey: " << p.second->clientId << ", "
                                                                                           << p.second->shmKey());
                errcode = CONNECT_STATUS_ALREADY_CONNECTED;
                break;
            }
        }
    }

    // Fill up proxy info
    auto consumerProxy = cit->second;
    consumerProxy->set_details(*cp);
    consumerProxy->on_connect(errcode);  // Send CtrlConnectResponse back

    if (errcode != CONNECT_STATUS_OK) {
        unregister_client(clientSock);
    } else {  // Register proxy with data sources
        for (auto&& dataSource : dataSources) {
            dataSource->register_consumer(consumerProxy);
        }
    }
}

/*
 * PubControlChannel->Publisher->ConsumerProxy::on_disconnect
 */
void Publisher::on_disconnect(int clientSock, Header* /*pHeader*/, CtrlDisconnect* /*dcp*/) {
    MIDAS_LOG_INFO("Received Disconnect");

    auto cit = consumers.find(clientSock);
    if (cit != consumers.end()) {
        cit->second->on_disconnect();
    }
    unregister_client(clientSock);
}

/*
 * PubControlChannel->Publisher::on_subscribe
 * ConsumerProxy is registered as a subscriber of the symbol being requested.
 */
void Publisher::on_subscribe(int clientSock, Header* /*pHeader*/, CtrlSubscribe* sp) {
    auto cit = consumers.find(clientSock);
    if (consumers.end() == cit) {
        // CtrlSubscribe is a "packed" struct, its fields, like exchange, flags, cannot be bound.
        auto const& exchange = sp->exchange;
        auto const& flags = sp->flags;
        MIDAS_LOG_ERROR("Received subscribe - Symbol: " << sp->symbol << "." << exchange << ", Flags: " << flags
                                                        << ", Client: UNKNOWN, Sock: " << clientSock);
        return;
    }

    auto const& exchange = sp->exchange;
    auto const& flags = sp->flags;
    MIDAS_LOG_INFO("Received subscribe - Symbol: " << sp->symbol << ", Flags: " << flags
                                                   << ", Client: " << cit->second->clientId
                                                   << ", Sock: " << clientSock);

    // verify that there is a cache for the requested exchange
    if (bookCaches.count(exchange)) {
        commandQueues[clientSock].emplace_back(
            std::make_tuple(MdCommand::subscribe, sp->symbol, exchange, sp->flags));
    } else {
        // Send back an error response: exchange doesn't exist for symbol
        cit->second->send_subscribe_response(SUBSCRIBE_STATUS_BAD_EXCHANGE, UINT16_MAX, sp->symbol, sp->exchange);
    }
}

// Called indirectly on the control channel thread, as part of housekeeping
bool Publisher::do_subscribe(int clientSock, char const* symbol, uint16_t exch, uint32_t flags) {
    static uint8_t nextSource = 0;

    auto cit = consumers.find(clientSock);
    if (consumers.end() == cit) {
        MIDAS_LOG_ERROR("Servicing subscribe - Symbol: " << symbol << ", Flags: " << flags
                                                         << ", Client: UNKNOWN, Sock: " << clientSock);
        return false;
    }
    MIDAS_LOG_INFO("Servicing subscribe - Symbol: " << symbol << ", Flags: " << flags << ", Client: "
                                                    << cit->second->clientId << ", Sock: " << clientSock);

    // Handle subscription here: no need to validate -- that occurred above in on_subscribe
    // before being place on the command queue to be serviced here
    {
        // Distribute subscription to sources in round robin manner
        auto dataSource = dataSources[nextSource++ % dataSources.size()];
        string subscribeKey{symbol};
        if (subscriptionBySource.count(subscribeKey) > 0) {
            dataSource = subscriptionBySource.at(subscribeKey);
        } else {
            subscriptionBySource.insert(std::make_pair(subscribeKey, dataSource));  // cache the source, always use it
        }

        if (dataSource->ready()) {
            dataSource->subscribe(subscribeKey, cit->second);
            cit->second->count_subscribe(symbol, exch);
        } else {
            MIDAS_LOG_INFO("Data source not ready to subscribe - Symbol: " << symbol << ", Flags: " << flags
                                                                           << ", Client: " << cit->second->clientId
                                                                           << ", Sock: " << clientSock);
            return false;
        }
    }
    return true;
}

/*
 * PubControlChannel->Publisher::on_unsubscribe
 * ConsumerProxy is Deregistered from list of subscribers of the symbol being unsubscribed.
 */
void Publisher::on_unsubscribe(int clientSock, Header* /*pHeader*/, CtrlUnsubscribe* usp) {
    auto cit = consumers.find(clientSock);
    if (consumers.end() == cit) {
        MIDAS_LOG_ERROR("Received unsubscribe - Symbol: " << usp->symbol << ", Client: UNKNOWN, Sock: " << clientSock);
        return;
    }

    MIDAS_LOG_ERROR("Received unsubscribe - Symbol: " << usp->symbol << ", Client: " << cit->second->clientId
                                                      << ", Sock: " << clientSock);

    // verify that there is a cache for the requested exchange
    if (bookCaches.count(usp->exchange)) {
        commandQueues[clientSock].emplace_back(std::make_tuple(MdCommand::unsubscribe, usp->symbol, usp->exchange, 0));
    } else {
        MIDAS_LOG_ERROR("Received unsubsribe for exchange with no cache: " << usp->symbol);
    }
    return;
}

// Called indirectly by the ControlChannel thread, as part of housekeeping
void Publisher::do_unsubscribe(int clientSock, char const* symbol, uint16_t exch) {
    auto cit = consumers.find(clientSock);
    if (consumers.end() == cit) {
        MIDAS_LOG_ERROR("Servicing unsubscribe - Symbol: " << symbol << ", Client: UNKNOWN, Sock: " << clientSock);
        return;
    }

    MIDAS_LOG_INFO("Servicing unsubscribe - Symbol: " << symbol << ", Client: " << cit->second->clientId
                                                      << ", Sock: " << clientSock);
    {
        string key{symbol};
        auto sit = subscriptionBySource.find(key);
        if (subscriptionBySource.end() == sit) {
            MIDAS_LOG_ERROR("Servicing unsubscribe - Symbol: " << symbol << ", instrument NOT FOUND !");
            return;
        }
        sit->second->unsubscribe(key, cit->second);
        cit->second->count_unsubscribe(symbol, exch);
    }
}

// Called by the ControlChannel thread
void Publisher::on_house_keeping() {
    time_t now = time(nullptr);
    // Execute pending commands
    for (auto&& cq : commandQueues) {
        int clientSock = cq.first;
        CommandQueueT& cmdq = cq.second;
        CommandQueueT::iterator cmdqit = cmdq.begin();
        uint32_t count = 0;
        CommandQueueT failedCmds;
        while (cmdqit != cmdq.end() && count < houseKeepingBatch) {
            std::tuple<MdCommand, std::string, uint16_t, uint32_t> const& cmd = *cmdqit;
            bool rc = true;
            if (std::get<0>(cmd) == MdCommand::subscribe) {
                rc = do_subscribe(clientSock, std::get<1>(cmd).c_str(), std::get<2>(cmd), std::get<3>(cmd));
            } else if (std::get<0>(cmd) == MdCommand::unsubscribe) {
                do_unsubscribe(clientSock, std::get<1>(cmd).c_str(), std::get<2>(cmd));
            }
            if (!rc) {
                failedCmds.emplace_back(cmd);  // Will retry them again later on
            }
            count += 1;
            ++cmdqit;
        }

        cmdq.erase(cmdq.begin(), cmdqit);  // remove commands executed

        if (!failedCmds.empty()) {  // enqueue commands which were not successfully executed
            for (auto&& retryCmd : failedCmds) {
                cmdq.emplace_back(retryCmd);
            }
        }
    }

    // Allow data sources to do their own housekeeping operations
    for (auto it = dataSources.begin(); it != dataSources.end(); ++it) {
        (*it)->on_call_house_keeping();
    }

    // Check on heartbeat situation
    std::vector<int> clientsNoHB;
    for (auto it = consumers.begin(); it != consumers.end(); ++it) {
        if (it->second->lastHeartbeat > 0) {
            if (now > controlChannel->maxMissingHB + it->second->lastHeartbeat) {
                MIDAS_LOG_INFO("No heartbeat from ClientID: " << it->second->clientId
                                                              << ", Sock: " << it->second->sockfd()
                                                              << ", Last_HB: " << it->second->lastHeartbeat);
                clientsNoHB.push_back(it->second->sockfd());
            } else {
                //                MIDAS_LOG_DEBUG("Last heartbeat from ClientID: " << it->second->clientId << ", Sock: "
                //                << it->second->sockfd() << ", received at " << it->second->lastHeartbeat);
            }
        }
    }
    for (auto cnohb : clientsNoHB) {
        unregister_client(cnohb);
    }
}

// Called by the ControlChannel thread
// Note that a book should only be touched by the thread which updates it.
void Publisher::clear_book() {
    for (auto& s : dataSources) {
        s->clear_all_book_cache();
    }
    return;
}
