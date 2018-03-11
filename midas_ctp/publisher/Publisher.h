#ifndef MIDAS_PUBLISHER_H
#define MIDAS_PUBLISHER_H

#include <net/raw/MdProtocol.h>
#include <unordered_map>
#include "net/shm/ShmCache.h"
#include "process/MidasProcessBase.h"

using namespace std;

class ConsumerProxy;      // Forward declaration
class PubControlChannel;  // Forward declaration
class DataSource;         // Forward declaration

class Publisher : public midas::MidasProcessBase {
public:
    enum struct MdCommand : uint8_t { subscribe = 0x00, unsubscribe = 0x01 };
    bool sendBookUpdate{false};
    uint32_t houseKeepingBatch{50};
    std::unordered_map<int, std::shared_ptr<ConsumerProxy>> consumers;
    std::shared_ptr<PubControlChannel> controlChannel;
    // enum       symbol,      exch,     flags
    using CommandQueueT = std::vector<std::tuple<MdCommand, std::string, uint16_t, uint32_t>>;
    std::unordered_map<int, CommandQueueT> commandQueues;
    std::unordered_map<uint16_t, std::unique_ptr<midas::ShmCache>> bookCaches;
    std::unordered_map<std::string, std::shared_ptr<DataSource>> subscriptionBySource;
    std::vector<std::shared_ptr<DataSource>> dataSources;
    uint64_t startTime{0};
    uint64_t session{0};
    std::string name;

public:
    Publisher() = delete;
    Publisher(int argc, char** argv);
    ~Publisher();

    void register_client(int clientSock, std::string const& clientIP, uint16_t clientPort);

    void unregister_client(int clientSock);

    void update_client_heartbeat(int clientSock);

    void on_connect(int clientSock, midas::Header*, midas::CtrlConnect*);

    void on_disconnect(int clientSock, midas::Header*, midas::CtrlDisconnect*);

    void on_subscribe(int clientSock, midas::Header*, midas::CtrlSubscribe*);

    bool do_subscribe(int clientSock, char const* symbol, uint16_t exch, uint32_t flags);

    void on_unsubscribe(int clientSock, midas::Header*, midas::CtrlUnsubscribe*);

    void do_unsubscribe(int clientSock, char const* symbol, uint16_t exch);

    void on_house_keeping();

    void clear_book();

    void create_book_cache(std::string const& key);

    uint8_t* book_cache_addr(uint16_t cacheId) const;

    midas::ShmCache* book_cache(uint16_t cacheId) const;

protected:
    void app_start() override;
    void app_stop() override;

private:
    bool configure();
    void init_admin();

private:
    string admin_meters(const string& cmd, const midas::TAdminCallbackArgs& args) const;
};

#endif
