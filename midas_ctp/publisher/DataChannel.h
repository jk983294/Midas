#ifndef MIDAS_DATA_CHANNEL_H
#define MIDAS_DATA_CHANNEL_H

#include <net/shm/CircularBuffer.h>
#include <net/shm/SharedMemory.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <pthread.h>
#include <cstdint>

class MdClient;  // fwd declare

class DataChannel : std::enable_shared_from_this<DataChannel> {
public:
    enum Mode { consumer = 0, publisher = 1 };

    MdClient* mdClient;
    Mode mode;
    bool stopped{true};
    bool poll{false};
    std::string cbReaderCore;
    pthread_t cbReaderThread;
    uint32_t queueSize{0};
    std::vector<std::string> queueNames;
    std::vector<midas::CircularBuffer<midas::SharedMemory>*> queues;
    std::atomic<uint64_t> read{0};
    std::atomic<uint64_t> readBytes{0};
    std::atomic<uint64_t> written{0};
    std::atomic<uint64_t> writtenBytes{0};
    std::atomic<uint64_t> dropped{0};
    std::atomic<uint64_t> droppedBytes{0};

public:
    DataChannel(std::string const& key, MdClient* client);

    DataChannel(std::string const& key);

    ~DataChannel();

    DataChannel(DataChannel const&) = delete;

    DataChannel& operator=(DataChannel const&) = delete;

    uint8_t create_queue(std::string const& name);

    bool start();

    bool stop();

    void loop_once();

    void run_loop();

    // read numBytes from shared memory
    uint8_t* write_prepare(uint32_t channelNo, uint32_t numBytes);

    // completes writing numBytes into shared memory
    void write_commit(uint32_t channelNo, uint32_t numBytes);

    static void* run(void* arg);
};

#endif
