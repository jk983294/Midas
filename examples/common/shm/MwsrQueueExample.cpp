#include <net/shm/MwsrQueue.h>

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <thread>

using namespace midas;

namespace {

struct MyType {
    uint64_t x = 0;
    double y = 0.0;
};

MyType build(uint32_t i_) { return {i_, static_cast<double>(i_)}; }
void print(const char* prefix_, MyType& i_) { fprintf(stderr, "%s: MyType {%lu, %.2f}\n", prefix_, i_.x, i_.y); }

inline void random_sleep() {
    if (rand() % 4 == 0) usleep(10);
}

const static uint32_t N_MSG = 10;
const static uint32_t N_THR = 4;
void mpscq_writer(MwsrQueue<MyType>& q_) {
    for (uint32_t i = 0; i < N_MSG; i++) {
        q_.put(build(i));
        random_sleep();
    }
}

void mpscq_reader(MwsrQueue<MyType>& q_) {
    uint32_t cnt = 0;
    MyType t;
    while (cnt < N_MSG * N_THR) {
        if (q_.get(t)) {
            print("<< ", t);
            cnt++;
        } else {
            usleep(10);  // queue empty, wait writer. or, may spin as well.
        }
    }
}

}  // namespace

int main() {
    srand(time(0));
    MwsrQueue<MyType> q;

    std::array<std::thread, N_THR> tws;
    for (uint32_t i = 0; i < N_THR; i++) tws[i] = std::thread{mpscq_writer, std::ref(q)};

    std::thread tr{mpscq_reader, std::ref(q)};

    for (uint32_t i = 0; i < N_THR; i++) tws[i].join();
    tr.join();

    return 0;
}
