#ifndef MIDAS_PUB_CONTROL_CHANNEL_H
#define MIDAS_PUB_CONTROL_CHANNEL_H

#include <net/shm/MwsrQueue.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

class Publisher;

class PubControlChannel {
public:
    Publisher* publisher;
    bool stopped{false};
    uint64_t session{0};
    int fdEpoll{-1};
    int fdControl{-1};
    uint32_t intervalHeartbeat{1};
    uint32_t maxMissingHB{5};
    int fdHeartbeatTimer{-1};
    uint32_t intervalKeepTicking{20};
    int fdKeepTickingTimer{-1};
    std::string ipPublisher;
    uint16_t portPublisher{0};
    pthread_t controlThread;
    bool connected{false};
    pthread_mutex_t connectedLock;
    pthread_cond_t connectedCV;

public:
    PubControlChannel(std::string const& key, Publisher* publisher_);

    PubControlChannel(PubControlChannel const&) = delete;

    PubControlChannel& operator=(PubControlChannel const&) = delete;

    ~PubControlChannel();

    bool start() {
        int rc = pthread_create(&controlThread, nullptr, run, this);
        return (rc == 0);
    }

    bool stop() {
        stopped = true;
        pthread_join(controlThread, nullptr);
        return stopped;
    }

    bool producer_server();

    void run_loop();

    static void* run(void* arg);

    void signal_connected();

    // read packetCount message packets where one message packet => [1xHeader + nxMsgs]
    int receive(int sockFd, int packetCount);
};

#endif
