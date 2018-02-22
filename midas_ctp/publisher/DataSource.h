#ifndef MIDAS_DATA_SOURCE_H
#define MIDAS_DATA_SOURCE_H

#include <midas/md/MdExchange.h>
#include <net/raw/MdProtocol.h>
#include <net/shm/CircularBuffer.h>
#include <net/shm/HeapMemory.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vector>

class ConsumerProxy;  // fwd declare
class Publisher;      // fwd declare

class DataSource {
public:
    uint8_t queueIndex;
    uint64_t mdSequence{0};
    std::string core;
    std::string scheduler;
    midas::CircularBuffer<midas::HeapMemory, uint8_t> circularBuffer;
    Publisher const& publisher;
    std::mutex consumerLock;
    std::vector<std::shared_ptr<ConsumerProxy>> consumers;
    bool canUnsubscribe{false};

public:
    DataSource(Publisher const& publisher, uint8_t queueIndex);

    virtual ~DataSource() = default;

    DataSource(DataSource const&) = delete;

    DataSource& operator=(DataSource const&) = delete;

    virtual bool ready() = 0;

    virtual void on_call_house_keeping() = 0;

    virtual bool poll_command_queue() = 0;

    virtual void subscribe(std::string const& key, std::shared_ptr<ConsumerProxy> const& consumerProxy) = 0;
    virtual void unsubscribe(std::string const& key, std::shared_ptr<ConsumerProxy> const& consumerProxy) = 0;

    void register_consumer(std::shared_ptr<ConsumerProxy> consumerProxy);
    std::size_t unregister_consumer(std::shared_ptr<ConsumerProxy> const& consumerProxy);

    virtual void clear_all_market_data() = 0;
    virtual void clear_market_data(std::string const& key) = 0;
    virtual void clear_all_book_cache() = 0;

    uint64_t step_sequence() { return ++mdSequence; }
    void clear_book_cache(void* userData);

    static std::vector<midas::MdExchange> participant_exchanges();

protected:
    // Call from a suitable implementation-specific callback/timer
    // to send a data heartbeat message to all enabled consumers
    bool send_data_heartbeat();

    // Called from register_consumer after adding to consumers
    virtual void register_consumer_hook(ConsumerProxy&) {}

    // Called from unregister_consumer before removing from consumers
    // Return number of subscriptions from which the consumer was removed
    virtual size_t unregister_consumer_hook(std::shared_ptr<ConsumerProxy> const& consumer) = 0;
};

#endif
