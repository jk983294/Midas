#include <net/shm/CircularBuffer.h>
#include <net/shm/HeapMemory.h>
#include <net/shm/SharedMemory.h>
#include <thread>

using namespace midas;

namespace {

void print(const char* prefix_, const uint8_t* buf_) { fprintf(stderr, "%s: {%s}\n", prefix_, buf_); }

static uint32_t N = 20;
static uint32_t L = 32;
template <typename CircularBufferType>
void rb_writer(CircularBufferType& rb_) {
    uint8_t* d = nullptr;
    for (uint32_t c = 1; c <= N;) {
        if (!rb_.get_write_pointer(d, L)) {
            usleep(10);  // queue full, wait reader. or, may spin as well.
        } else {
            memset(d, static_cast<char>('a' + c), L);
            d[L - 1] = '\0';
            rb_.commit_write(L);
            c++;
        }
    }
}

template <typename CircularBufferType>
void rb_reader(CircularBufferType& rb_) {
    uint8_t* d = nullptr;
    for (uint32_t c = 1; c <= N;) {
        if (!rb_.get_read_pointer(d, L)) {
            usleep(10);  // queue empty, wait writer. or, may spin as well.
        } else {
            print("<< ", d);
            rb_.commit_read(L);
            c++;
        }
    }
}

void run_heap() {
    uint32_t size = 256 * 1024;
    HeapMemory space{size};
    CircularBuffer<HeapMemory> rb{std::move(space), true};

    std::thread tw{rb_writer<decltype(rb)>, std::ref(rb)};
    std::thread tr{rb_reader<decltype(rb)>, std::ref(rb)};
    tw.join();
    tr.join();
}

void run_shm() {
    uint32_t size = 256 * 1024;
    // writer creates shared memory and reader attaches
    SharedMemory shm_c1 = SharedMemory::create_shared_memory("example_shm_rb1", size);
    SharedMemory shm_a1 = SharedMemory::attach_shared_memory("example_shm_rb1");
    CircularBuffer<SharedMemory> rb_w1{std::move(shm_c1), true};
    CircularBuffer<SharedMemory> rb_r1{std::move(shm_a1)};

    std::thread tw1{rb_writer<decltype(rb_w1)>, std::ref(rb_w1)};
    std::thread tr1{rb_reader<decltype(rb_r1)>, std::ref(rb_r1)};
    tw1.join();
    tr1.join();

    // attacher of shared memory can act as writer as well.
    SharedMemory shm_c2 = SharedMemory::create_shared_memory("example_shm_rb2", size);
    SharedMemory shm_a2 = SharedMemory::attach_shared_memory("example_shm_rb2");
    CircularBuffer<SharedMemory> rb_w2{std::move(shm_a2), true};
    CircularBuffer<SharedMemory> rb_r2{std::move(shm_c2)};

    std::thread tw2{rb_writer<decltype(rb_w2)>, std::ref(rb_w2)};
    std::thread tr2{rb_reader<decltype(rb_r2)>, std::ref(rb_r2)};
    tw2.join();
    tr2.join();
}

}  // namespace

int main() {
    // Queue on heap
    run_heap();

    // Queue on shared memory
    run_shm();

    return 0;
}
