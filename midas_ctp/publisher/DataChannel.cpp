#include <midas/MidasConfig.h>
#include <midas/md/MdDefs.h>
#include <net/raw/MdProtocol.h>
#include <utils/MidasUtils.h>
#include "DataChannel.h"

using namespace std;
using namespace midas;

DataChannel::DataChannel(std::string const& keyConfig) {
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
        try {
            // first, try to create the shared memory file, throw if file with same shmName exists.
            midas::SharedMemory shm = midas::SharedMemory::create_shared_memory(name, queueSize);
            cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm), true});
        } catch (midas::MidasException const& ex) {
            // if create throws, try to recover from existing file.
            midas::SharedMemory shm = midas::SharedMemory::reclaim_shared_memory(name);
            cb.reset(new midas::CircularBuffer<midas::SharedMemory>{std::move(shm)});
        }
        queues.push_back(cb.release());
        queueNames.emplace_back(name);
        MIDAS_LOG_INFO("Created a shared memory queue with shmName " << name);
    } catch (midas::MidasException const& ex) {
        MIDAS_LOG_ERROR("Failed to create shared memory queue with shmName " << name << ", error: " << ex.what());
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

bool DataChannel::start() { return true; }

bool DataChannel::stop() { return true; }
