#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <utils/MidasUtils.h>
#include "Consumer.h"
#include "SubDataChannel.h"

using namespace std;
using namespace midas;

SubDataChannel::SubDataChannel(std::string const& keyConfig, Consumer* client) : consumer(client) {
    poll = midas::get_cfg_value<bool>(keyConfig, "poll", false);
    MIDAS_LOG_INFO("data channel cfg poll " << poll);
}

SubDataChannel::~SubDataChannel() {
    MIDAS_LOG_INFO("Destroying data channel");
    auto n = queueNames.begin();
    for (auto&& v : queues) {
        MIDAS_LOG_INFO("Destroying shared memory queue " << *(n++));
        delete v;
    }
}

uint8_t SubDataChannel::create_queue(std::string const& name) {
    for (auto&& it : queueNames) {
        if (it == name) {
            MIDAS_LOG_INFO("Shared memory queue for " << name << " exists...");
            return 0;
        }
    }

    std::unique_ptr<midas::CircularBuffer<midas::SharedMemory>> cb;
    try {
        midas::SharedMemory shm = midas::SharedMemory::attach_shared_memory(name);
        cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm)});
        queues.push_back(cb.release());
        queueNames.emplace_back(name);
        MIDAS_LOG_INFO("Created a shared memory queue with shmName " << name);
    } catch (midas::MidasException const& ex) {
        MIDAS_LOG_ERROR("Failed to create shared memory queue with shmName " << name << ", error: " << ex.what());
        return CONNECT_STATUS_SHARED_MEMORY_FAILURE;
    }

    return 0;
}

bool SubDataChannel::start() {
    stopped = false;
    if (poll) {
        MIDAS_LOG_INFO("start data channel thread.");
        int rc = pthread_create(&cbReaderThread, nullptr, run, this);
        if (0 == rc) {
            sched_yield();
        } else {
            MIDAS_LOG_ERROR("pthread_create failed: " << rc);
        }
        return (rc == 0);
    }
    return true;
}

bool SubDataChannel::stop() {
    if (!stopped) {
        stopped = true;
        if (poll) {
            pthread_join(cbReaderThread, nullptr);
        }
    }
    return true;
}

void SubDataChannel::loop_once() {
    if (stopped) return;

    for (auto it = queues.begin(); it != queues.end() && !stopped; ++it) {
        midas::CircularBuffer<midas::SharedMemory>* cb = *it;
        uint8_t* ptr = nullptr;
        Header* header = nullptr;
        for (;;) {
            if (cb->get_read_pointer(ptr, sizeof(Header)) == sizeof(Header)) {
                header = (Header*)ptr;
                uint8_t* pData = nullptr;
                // cannot commit header here, since we still need it ...
                uint32_t size2read = (uint32_t)(sizeof(Header) + header->size);
                if (header->size > 0 && cb->get_read_pointer(pData, size2read) == size2read) {
                    pData += sizeof(Header);
                    for (auto c = 0; c < header->count; ++c) {
                        switch (*pData) {
                            case DATA_TRADING_ACTION_TYPE:
                                consumer->on_trading_action(header, (DataTradingAction*)pData);
                                break;
                            case DATA_BOOK_CHANGED_TYPE:
                                consumer->on_book_changed(header, (DataBookChanged*)pData);
                                break;
                            case DATA_BOOK_REFRESHED_TYPE:
                                consumer->on_book_refreshed(header, (DataBookRefreshed*)pData);
                                break;
                            default:
                                break;
                        }
                        pData += 1;             // advance by 1 byte to size field
                        pData += (*pData - 1);  // advance to next message
                    }
                    cb->commit_read(size2read);

                    read += 1;
                    readBytes += header->size;
                } else if (header->size == 0) {  // Merely heartbeat
                    DataHeartbeat dataHeartbeat;
                    dataHeartbeat.hb.sessionId = header->session;
                    dataHeartbeat.hb.streamId = header->streamId;
                    dataHeartbeat.hb.timestamps.producerTransmit = header->transmitTimestamp;
                    consumer->on_data_heartbeat(header, &dataHeartbeat);
                    cb->commit_read(sizeof(Header));
                } else {
                    // Should not happen - as writing of (header + payload) is signalled together
                    cb->commit_read(sizeof(Header));
                }
            } else {
                break;  // Read from next queue
            }
        }
    }
}

void SubDataChannel::run_loop() {
    while (!stopped) {
        loop_once();
    }
}

void* SubDataChannel::run(void* arg) {
    SubDataChannel* dc = static_cast<SubDataChannel*>(arg);
    MIDAS_LOG_INFO("Starting data channel for consumer, thread id: " << midas::get_tid());
    usleep(1000);
    dc->run_loop();
    return nullptr;
}
