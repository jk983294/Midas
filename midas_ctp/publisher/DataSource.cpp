#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include "ConsumerProxy.h"
#include "DataSource.h"

using namespace std;
using namespace midas;

DataSource::DataSource(Publisher const& publisher_, uint8_t queueIndex_)
    : queueIndex(queueIndex_), circularBuffer(midas::HeapMemory{16 * 1048576}, true), publisher(publisher_) {
    core = midas::get_cfg_value<std::string>("ctp", "affinity");
    scheduler = midas::get_cfg_value<std::string>("ctp", "scheduler");
    canUnsubscribe = midas::get_cfg_value<bool>("ctp", "can_unsubscribe", false);
}

/**
 * called from admin thread to clear book cache.
 * writes a message to the source's ring buffer,
 * which must be honoured in the context of main loop of the data source's dedicated thread.
 * @param userData
 */
void DataSource::clear_book_cache(void* userData) {
    uint8_t* d = nullptr;
    if (circularBuffer.get_write_pointer(d, sizeof(Header) + sizeof(AdminBookClear)) ==
        (sizeof(Header) + sizeof(AdminBookClear))) {
        Header* hp = (Header*)d;  // in-memory event passing, we don't care other fields
        hp->count = 1;
        hp->size = sizeof(AdminBookClear);
        AdminBookClear* abc = (AdminBookClear*)((uint8_t*)d + sizeof(Header));
        abc->type = ADMIN_BOOK_CLEAR_TYPE;
        abc->msgSize = sizeof(AdminBookClear);
        abc->ticker = userData;
        circularBuffer.commit_write(sizeof(Header) + sizeof(AdminBookClear));
    }
    return;
}

// Called by the PubControlChannel thread
void DataSource::register_consumer(std::shared_ptr<ConsumerProxy> consumer) {
    const auto c_beg = std::begin(consumers);
    const auto c_end = std::end(consumers);
    if (std::find(c_beg, c_end, consumer) == c_end) {
        {
            std::lock_guard<std::mutex> g(consumerLock);
            consumers.emplace_back(consumer);
        }
        register_consumer_hook(*consumer);
    }
}

// Called from PubControlChannel thread
size_t DataSource::unregister_consumer(std::shared_ptr<ConsumerProxy> const& consumer) {
    const auto c_beg = std::begin(consumers);
    const auto c_end = std::end(consumers);
    const auto result = std::find(c_beg, c_end, consumer);
    if (result != c_end) {
        size_t rc = unregister_consumer_hook(consumer);
        {
            std::lock_guard<std::mutex> g(consumerLock);
            consumers.erase(result);
        }
        return rc;
    }
    return 0;
}

// Called from Market Data processing thread
bool DataSource::send_data_heartbeat() {
    std::lock_guard<std::mutex> g(consumerLock);
    for (auto& c : consumers) {
        c->send_data_heartbeat(queueIndex, step_sequence());
    }

    return true;
}

std::vector<MdExchange> DataSource::participant_exchanges() {
    std::vector<MdExchange> exchanges;
    MdExchange me;
    me.exchangeDepth = 5;
    me.bytesPerProduct = 16;
    me.exchange = ExchangeCFFEX;
    exchanges.push_back(me);
    me.exchange = ExchangeCZCE;
    exchanges.push_back(me);
    me.exchange = ExchangeDCE;
    exchanges.push_back(me);
    me.exchange = ExchangeSHFE;
    exchanges.push_back(me);
    return exchanges;
}
