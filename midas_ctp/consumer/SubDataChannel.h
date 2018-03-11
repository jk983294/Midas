#ifndef MIDAS_SUB_DATA_CHANNEL_H
#define MIDAS_SUB_DATA_CHANNEL_H

#include <net/shm/CircularBuffer.h>
#include <net/shm/SharedMemory.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Consumer;  // fwd declare

class SubDataChannel {
public:
    Consumer* consumer;
    bool stopped{true};
    bool poll{false};  // true means it will create thread within data channel
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
    SubDataChannel(std::string const& key, Consumer* client);

    ~SubDataChannel();

    SubDataChannel(SubDataChannel const&) = delete;

    SubDataChannel& operator=(SubDataChannel const&) = delete;

    uint8_t create_queue(std::string const& name);

    bool start();

    bool stop();

    void loop_once();

    void run_loop();

    static void* run(void* arg);
};

#endif
