#ifndef MIDAS_CONSUMER_PROXY_H
#define MIDAS_CONSUMER_PROXY_H

#include <net/raw/MdProtocol.h>
#include <atomic>
#include <cstdint>
#include <ctime>
#include <memory>
#include <mutex>
#include <set>
#include <string>

class DataChannel;  // Forward declaration
class MdTicker;     // Forward declaration
class Publisher;    // Forward declaration

/**
 * For each MdConsumer a corresponding ConsumerProxy is created on Publisher
 * ConsumerProxy is what interacts with Publisher on behalf of actual consumer
 */
class ConsumerProxy {
public:
    Publisher const *publisher;
    int sockFdConsumer;
    std::string ipConsumer;
    uint16_t portConsumer;
    pid_t pidConsumer{0};
    uint8_t clientId{0};
    std::string userConsumer;
    std::string shmKeyConsumer;
    std::mutex consumerProxyLock;
    std::atomic<uint64_t> controlSeqNo{0};
    uint8_t numDataQueues;
    std::unique_ptr<DataChannel> dataChannel;
    time_t lastHeartbeat{0};
    uint32_t subscriptionFlags{0};
    std::set<std::string> requestedSubs;

public:
    ConsumerProxy(int sockFdConsumer_, std::string const &ipConsumer_, uint16_t portConsumer_,
                  Publisher const *publisher_, DataChannel *dataChannel_, uint8_t numDataChannels_);

    ~ConsumerProxy();

    ConsumerProxy(ConsumerProxy const &) = delete;

    ConsumerProxy &operator=(ConsumerProxy const &) = delete;

    /*
     * ->ControlChannel-on-Consumer
     */
    void send_subscribe_response(MdTicker const &ticker);
    void send_subscribe_response(uint8_t status, uint16_t locate, char const *mdTick, uint16_t mdExch);

    /*
     * ->ControlChannel-on-Consumer
     */
    void send_unsubscribe_response(MdTicker const &ticker);

    /*
     * <<<Shared Memory Queue>>>->DataChannel-on-Consumer
     */
    void send_book_refreshed(MdTicker const &ticker);

    /*
     * -><<<Shared Memory Queue>>>->DataChannel-on-Consumer
     */
    void send_book_changed(MdTicker const &ticker);

    /*
     * <<<Shared Memory Queue>>>->DataChannel-on-Consumer
     */
    void send_trading_action(MdTicker const &ticker, uint16_t exch);

    /*
     * -><<<Shared Memory Queue>>>->DataChannel-on-Consumer
     */
    void send_data_heartbeat(uint8_t queueIndex, uint64_t stepSequence);

    int sockfd() const { return sockFdConsumer; }

    std::string const &ip() const { return ipConsumer; }

    uint16_t port() const { return portConsumer; }

    pid_t pid() const { return pidConsumer; }

    std::string const &user() const { return userConsumer; }

    std::string const &shmKey() const { return shmKeyConsumer; }

    /*
     * Following methods are intended for use by Publisher only
     */
    void set_details(midas::CtrlConnect const &ctrl);

    /*
     * ControlChannel->Publisher->ConsumerProxy::update_heartbeat
     */
    void update_heartbeat() { lastHeartbeat = time(nullptr); }

    /*
     * ControlChannel->Publisher->ConsumerProxy::on_connect
     */
    void on_connect(uint8_t status);

    /*
     * ControlChannel->Publisher->ConsumerProxy::on_disconnect
     */
    void on_disconnect();  // Publisher calls into on_disconnect

    /*
     * ControlChannel->Publisher->ConsumerProxy::on_subscribe
     */
    void count_subscribe(std::string const &symbol, uint16_t exch);  // no lock necessary

    /*
     * ControlChannel->Publisher->ConsumerProxy::on_subscribe
     */
    void count_unsubscribe(std::string const &symbol, uint16_t exch);  // no lock necessary

private:
    void send_connect_response(uint8_t status);
    void send_disconnect_response();
};

#endif
