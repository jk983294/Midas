#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <utils/MidasUtils.h>
#include "DataChannel.h"
#include "MdClient.h"

using namespace std;
using namespace midas;

DataChannel::DataChannel(std::string const& keyConfig, MdClient* client)
    : mdClient(client), mode(DataChannel::consumer) {
    poll = midas::get_cfg_value<bool>(keyConfig, "poll", true);
    cbReaderCore = midas::get_cfg_value<bool>(keyConfig, "cb_reader_core");
    MIDAS_LOG_INFO("data channel cfg poll " << poll << " cbReaderCore " << cbReaderCore);
}

DataChannel::DataChannel(std::string const& keyConfig) : mode(DataChannel::publisher) {
    queueSize = midas::get_cfg_value<uint32_t>(keyConfig, "shm_queue_size", 16 * 1024 * 1024);
    MIDAS_LOG_INFO("data channel cfg queueSize " << queueSize);
}

DataChannel::~DataChannel() {
    MIDAS_LOG_INFO("Destroying data channel");
    auto n = queueNames.begin();
    for (auto&& v : queues) {
        MIDAS_LOG_INFO("Destroying shared memory queue " << *(n++));
        delete v;
    }
}

uint8_t DataChannel::create_queue(std::string const& name) {
    for (auto&& it : queueNames) {
        if (it == name) {
            MIDAS_LOG_INFO("Shared memory queue for " << name << " exists...");
            return 0;
        }
    }

    std::unique_ptr<midas::CircularBuffer<midas::SharedMemory>> cb;
    try {
        if (mode == DataChannel::publisher) {
            try {
                // first, try to create the shared memory file, throw if file with same name exists.
                midas::SharedMemory shm = midas::SharedMemory::create_shared_memory(name, queueSize);
                cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm), true});
            } catch (midas::MidasException const& ex) {
                // if create throws, try to recover from existing file.
                midas::SharedMemory shm = midas::SharedMemory::reclaim_shared_memory(name);
                cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm)});
            }
        } else {
            midas::SharedMemory shm = midas::SharedMemory::attach_shared_memory(name);
            cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm)});
        }
        queues.push_back(cb.release());
        queueNames.emplace_back(name);
        MIDAS_LOG_INFO("Created a shared memory queue with name " << name);
    } catch (midas::MidasException const& ex) {
        MIDAS_LOG_ERROR("Failed to create shared memory queue with name " << name << ", error: " << ex.what());
        return CONNECT_STATUS_SHARED_MEMORY_FAILURE;
    }

    return 0;
}

uint8_t* DataChannel::write_prepare(uint32_t channelNo, uint32_t numBytes) {
    midas::CircularBuffer<midas::SharedMemory>* cb = queues[channelNo];
    uint8_t* d = nullptr;
    if (cb->get_write_pointer(d, numBytes) == numBytes) {
        return d;
    } else {
        dropped += 1;
        droppedBytes += numBytes;
        return nullptr;
    }
}

void DataChannel::write_commit(uint32_t channelNo, uint32_t numBytes) {
    midas::CircularBuffer<midas::SharedMemory>* cb = queues[channelNo];
    cb->commit_write(numBytes);
    written += 1;
    writtenBytes += numBytes;
}

bool DataChannel::start() {
    stopped = false;
    if (poll) {
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

bool DataChannel::stop() {
    if (!stopped) {
        stopped = true;
        if (poll) {
            pthread_join(cbReaderThread, nullptr);
        }
    }
    return true;
}

void DataChannel::loop_once() {
    if (mode == DataChannel::publisher) return;
    if (stopped) return;

    for (auto it = queues.begin(); it != queues.end() && !stopped; ++it) {
        midas::CircularBuffer<midas::SharedMemory>* cb = *it;
        uint8_t* ptr = nullptr;
        Header* header = nullptr;
        for (;;) {  // Read as much as there is
            if (cb->get_read_pointer(ptr, sizeof(Header)) == sizeof(Header)) {
                header = (Header*)ptr;
                uint8_t* pData = nullptr;
                // cannot commit header here, since we still need it ...
                uint32_t size_to_read = (uint32_t)(sizeof(Header) + header->size);
                if (header->size > 0 && cb->get_read_pointer(pData, size_to_read) == size_to_read) {
                    pData += sizeof(Header);
                    for (auto c = 0; c < header->count; ++c) {
                        switch (*pData) {
                            case DATA_TRADING_ACTION_TYPE:
                                mdClient->on_trading_action(header, (DataTradingAction *) pData);
                                break;
                            case DATA_BOOK_CHANGED_TYPE:
                                mdClient->on_book_changed(header, (DataBookChanged *) pData);
                                break;
                            case DATA_BOOK_REFRESHED_TYPE:
                                mdClient->on_book_refreshed(header, (DataBookRefreshed *) pData);
                                break;
                            default:
                                break;
                        }
                        pData += 1;             // advance by 1 byte to size field
                        pData += (*pData - 1);  // advance to next message
                    }
                    cb->commit_read(size_to_read);

                    read += 1;
                    readBytes += header->size;
                } else if (header->size == 0) {  // Merely heartbeat
                    DataHeartbeat dataHeartbeat;
                    dataHeartbeat.hb.sessionId = header->session;
                    dataHeartbeat.hb.streamId = header->streamId;
                    dataHeartbeat.hb.timestamps.producerTransmit = header->xmitts;  // add the transmit on-the-fly
                    mdClient->on_data_heartbeat(header, &dataHeartbeat);
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

void DataChannel::run_loop() {
    while (!stopped) {
        loop_once();
    }
}

void* DataChannel::run(void* arg) {
    DataChannel* dc = static_cast<DataChannel*>(arg);
    assert(dc->mode == DataChannel::consumer);
    MIDAS_LOG_INFO("Starting data channel for consumer, thread id: " << midas::get_tid());
    usleep(1000);
    dc->run_loop();
    return nullptr;
}
