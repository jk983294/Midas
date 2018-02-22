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

class PubControlChannel : std::enable_shared_from_this<PubControlChannel> {
public:
    Publisher* publisher;
    bool stopped{false};
    pid_t pid{0};
    uint64_t session{0};
    std::atomic<uint64_t> sequence{0};
    int fdEpoll{-1};
    int fdControl{-1};
    uint32_t intervalHeartbeat{1};
    uint32_t maxMissingHB{5};
    uint32_t pauseMillisecsAfterDisconnect{250};
    int fdHeartbeatTimer{-1};
    uint32_t intervalKeepTicking{20};
    int fdKeepTickingTimer{-1};
    int fdPeriodicalTimer{-1};
    std::string ipPublisher;
    uint16_t portPublisher{0};
    std::string ipConsumer;
    std::string controlCore;
    pthread_t controlThread;
    int connectTimeout{0};
    std::string controlQueue;
    uint32_t controlQueueSize{0};
    std::atomic<uint64_t> controlSeqNo{0};
    uint32_t heartbeatInterval{0};
    using mdsymT = std::tuple<std::string, uint16_t>;
    midas::MwsrQueue<mdsymT> reqSubscribe;
    midas::MwsrQueue<mdsymT> reqUnsubscribe;
    uint32_t numReqAtATime{50};
    bool connected{false};
    pthread_mutex_t connectedLock;
    pthread_cond_t connectedCV;

public:
    PubControlChannel(std::string const& key, Publisher* publisher_);

    PubControlChannel(PubControlChannel const&) = delete;

    PubControlChannel& operator=(PubControlChannel const&) = delete;

    ~PubControlChannel();

    std::shared_ptr<PubControlChannel> get_ptr() { return shared_from_this(); }

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
