#ifndef MIDAS_SUB_CONTROL_CHANNEL_H
#define MIDAS_SUB_CONTROL_CHANNEL_H

#include <net/shm/MwsrQueue.h>
#include <pthread.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

class Subscriber;
class Consumer;

class SubControlChannel {
public:
    Consumer* consumer;
    bool stopped{false};
    uint64_t session{0};
    int fdEpoll{-1};
    int fdControl{-1};
    uint32_t intervalHeartbeat{1};
    uint32_t pauseMillisecsAfterDisconnect{250};
    int fdHeartbeatTimer{-1};
    int fdKeepTickingTimer{-1};
    int fdPeriodicalTimer{-1};
    std::string ipPublisher;
    uint16_t portPublisher{0};
    std::string ipConsumer;
    pthread_t controlThread;
    int connectTimeout{0};
    std::atomic<uint64_t> controlSeqNo{0};
    using mdsymT = std::tuple<std::string, uint16_t>;
    midas::MwsrQueue<mdsymT> reqSubscribe;
    midas::MwsrQueue<mdsymT> reqUnsubscribe;
    uint32_t numReqAtATime{50};
    bool connected{false};
    pthread_mutex_t connectedLock;
    pthread_cond_t connectedCV;

public:
    SubControlChannel(std::string const& key, Consumer* consumer_);

    SubControlChannel(SubControlChannel const&) = delete;

    SubControlChannel& operator=(SubControlChannel const&) = delete;

    ~SubControlChannel();

    bool start() {
        int rc = pthread_create(&controlThread, nullptr, run, this);
        return (rc == 0);
    }

    bool stop() {
        stopped = true;
        pthread_join(controlThread, nullptr);
        return stopped;
    }

    bool connect();

    bool wait_until_connected();

    bool disconnect();

    int send_heartbeat();

    void run_loop();

    static void* run(void* arg);

    void signal_connected();

    int recv(int sockfd, int npkts);  // read 'npkts' message packets where one message packet => [1xHeader + nxMsgs]

    int send_connect();

    int send_disconnect();

    uint8_t send_subscribe(std::string const& mdTick, uint16_t mdExch);

    uint8_t send_unsubscribe(std::string const& mdTick, uint16_t mdExch);
};

#endif
