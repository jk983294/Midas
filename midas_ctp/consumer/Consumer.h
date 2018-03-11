#ifndef MIDAS_CONSUMER_H
#define MIDAS_CONSUMER_H

#include <net/raw/MdProtocol.h>
#include <unordered_map>
#include "MySymbol.h"
#include "ShmCacheConsumer.h"
#include "Subscription.h"
#include "net/shm/ShmCache.h"
#include "process/MidasProcessBase.h"

using namespace std;

class SubDataChannel;     // Forward declaration
class SubControlChannel;  // Forward declaration

class Consumer : public midas::MidasProcessBase {
public:
    bool failedSubscription{false};
    std::shared_ptr<SubControlChannel> controlChannel;
    std::shared_ptr<SubDataChannel> dataChannel;
    uint64_t startTime{0};
    uint64_t session{0};
    pid_t pid{0};
    std::string shmName;
    std::string userName;
    std::string password;
    uint32_t subscriptionFlags{0};
    uint16_t clientId{0};
    uint64_t bookChangedEventCount{0};
    uint64_t bookRefreshedEventCount{0};
    std::unordered_map<uint16_t, std::unique_ptr<ShmCacheConsumer>> bookCaches;  // exchCode -> cache
    std::recursive_mutex subscriptionLock;
    std::shared_ptr<SubTicker> subscriptionByCode[UINT8_MAX][UINT16_MAX];
    std::unordered_map<std::string, std::shared_ptr<SubTicker>> subscriptionByName;
    bool connected{false};
    void *globalUserData{nullptr};
    uint32_t max_opt_snap_attempts{20};
    uint32_t max_lock_snap_attempts{5};
    uint32_t reader_lock_timeout{5000};
    bool poll{true};
    std::unique_ptr<MySymbol> pSymbol;
    std::thread clientWorker_;

public:
    Consumer() = delete;
    Consumer(int argc, char **argv);
    ~Consumer();

    void client_logic();

    bool ready() const;

    uint8_t subscribe(Subscription **ppSub, std::string const &mdTick, uint16_t mdExch, void *userData,
                      bool duplicateOK = false);

    void resubscribe();

    uint8_t unsubscribe(Subscription **ppsub);

    uint8_t alloc_book(midas::MdBook *book, bool alloc = true);

    uint8_t alloc_book(uint16_t mdExch, midas::MdBook *book, bool alloc = true);

    void data_poll();

    void on_control_channel_connected();

    void on_control_channel_disconnected();

    void on_control_channel_established();

    void on_trading_action(midas::Header *, midas::DataTradingAction *);

    void on_book_refreshed(midas::Header *, midas::DataBookRefreshed *);

    void on_book_changed(midas::Header *, midas::DataBookChanged *);

    void on_data_heartbeat(midas::Header *, midas::DataHeartbeat *);

    void on_connect_response(midas::Header *, midas::CtrlConnectResponse *, bool *isConnected);

    void on_disconnect_response(midas::Header *, midas::CtrlDisconnectResponse *);

    void on_subscribe_response(midas::Header *, midas::CtrlSubscribeResponse *);

    void on_unsubscribe_response(midas::Header *, midas::CtrlUnsubscribeResponse *);

    void callback_periodically() {}

    uint32_t callback_interval() const { return 1000; }

    void callback_subscribe_response(char const *symbol, uint16_t exch, uint8_t statusSubscription,
                                     void * /*userData*/);
    void callback_book_changed(char const *symbol, uint16_t exch, midas::BookChanged const * /*bkchngd*/,
                               void *userData);
    void callback_book_refreshed(char const *symbol, uint16_t exch, void *userData);
    void callback_trade_action(char const *symbol, uint16_t exchange, midas::TradingAction const *trdst,
                               void *userData) {}

protected:
    void app_start() override;
    void app_stop() override;

private:
    bool configure();
    void init_admin();

private:
    string admin_meters(const string &cmd, const midas::TAdminCallbackArgs &args) const;
};

#endif
