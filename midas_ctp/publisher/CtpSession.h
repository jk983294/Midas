#ifndef MIDAS_CTP_SESSION_H
#define MIDAS_CTP_SESSION_H

#include <market/MarketManager.h>
#include <net/shm/MwsrQueue.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "CtpSessionConsumer.h"
#include "DataSource.h"
#include "MdTicker.h"

class Publisher;  // fwd declare

class CtpSession : public DataSource {
public:
    static midas::MdExchange ctpExchange;
    std::unordered_map<std::string, std::shared_ptr<MdTicker>> subscriptions;
    midas::MwsrQueue<std::shared_ptr<MdTicker>> refreshNeeded;
    std::shared_ptr<MarketManager<CtpSessionConsumer>> marketManager;

public:
    static MdExchange participant_exchanges();

    CtpSession(Publisher const& publisher, uint8_t queueIndex);

    ~CtpSession();

    CtpSession(CtpSession const&) = delete;

    CtpSession& operator=(CtpSession const&) = delete;

    void subscribe(std::string const& symbol, std::shared_ptr<ConsumerProxy> const& consumerProxy) override;

    void unsubscribe(std::string const& symbol, std::shared_ptr<ConsumerProxy> const& consumerProxy) override;

    void clear_all_market_data() override;

    void clear_all_book_cache() override;

    void on_call_house_keeping() override;

    bool poll_command_queue() override;

protected:
    std::size_t unregister_consumer_hook(std::shared_ptr<ConsumerProxy> const& consumerProxy) override;

private:
    bool locate(std::string const& exegySymbol, uint16_t& locator);
};

#endif
