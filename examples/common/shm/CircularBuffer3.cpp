#include <net/shm/CircularBuffer.h>
#include <net/shm/HeapMemory.h>
#include <net/shm/SharedMemory.h>
#include <thread>

using namespace midas;

namespace {
struct CharArray {
    uint64_t len = 0;
    char* addr = nullptr;

    CharArray() = default;
    CharArray(uint32_t i_) : len{i_} {
        addr = new char[len];
        memset(addr, 'p', len);
        addr[len - 1] = '\0';
    }
    CharArray(uint64_t len_, char* ptr_) : len{len_}, addr{ptr_} {}
    ~CharArray() {
        if (addr) {
            delete addr;
            addr = nullptr;
        }
    }

    CharArray(const CharArray&) = delete;
    CharArray& operator=(const CharArray&) = delete;
    CharArray(CharArray&& other_) : len{other_.len}, addr{other_.addr} { other_.addr = nullptr; }
    CharArray& operator=(CharArray&& other_) {
        len = other_.len;
        addr = other_.addr;
        other_.addr = nullptr;
        return *this;
    }
};

struct Serializer {
    /* CharArray.len indicates the length of the space that CharArray.addr points to.
     */
    static const size_t SIZE = 40960;

    static std::pair<bool, uint32_t> deserialize(const uint8_t* buffer_, uint32_t length_, CharArray& t_) {
        t_.len = *(reinterpret_cast<const uint64_t*>(buffer_));

        if (t_.len + sizeof(uint64_t) > length_) {
            return std::make_pair(false, 0);
        }
        t_.addr = new char[t_.len];
        memcpy(t_.addr, buffer_ + sizeof(uint64_t), t_.len);
        return std::make_pair(true, (uint32_t)(t_.len + sizeof(uint64_t)));
    }
    Buffer serialize(const CharArray& v_, uint8_t* space_ = nullptr) {
        uint8_t* dst = space_ ? space_ : &space[0];
        memcpy(dst, &v_.len, sizeof(uint64_t));
        memcpy(dst + sizeof(uint64_t), v_.addr, v_.len);
        return {dst, static_cast<uint32_t>(v_.len + sizeof(uint64_t))};
    }
    static uint32_t recordSize() { return 0; /* zero indicates variable record length. */ }

    uint8_t space[SIZE];
};

void print(const char* prefix_, const CharArray& i_) {
    fprintf(stderr, "%s: CharArray {%lu, %s}\n", prefix_, i_.len, i_.addr);
}

static uint32_t N = 20;
template <typename CircularBufferType>
void rb_writer(CircularBufferType& rb_) {
    for (uint32_t c = 1; c <= N;) {
        CharArray e{c};
        if (!rb_.write(e)) {
            usleep(10);  // queue full, wait reader. or, may spin as well.
        } else {
            c++;
        }
    }
}

template <typename CircularBufferType>
void rb_reader(CircularBufferType& rb_) {
    for (uint32_t c = 1; c <= N;) {
        CharArray e;
        bool r = rb_.read(e);
        if (!r) {
            usleep(10);  // queue empty, wait writer. or, may spin as well.
        } else {
            print("<< ", e);
            c++;
        }
    }
}

void run_heap() {
    uint32_t size = 256 * 1024;
    HeapMemory space{size};
    CircularBuffer<HeapMemory, uint8_t, NonFlatStructTraits<CharArray, Serializer>> rb{std::move(space), true};

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
    CircularBuffer<SharedMemory, uint8_t, NonFlatStructTraits<CharArray, Serializer>> rb_w1{std::move(shm_c1), true};
    CircularBuffer<SharedMemory, uint8_t, NonFlatStructTraits<CharArray, Serializer>> rb_r1{std::move(shm_a1)};

    std::thread tw1{rb_writer<decltype(rb_w1)>, std::ref(rb_w1)};
    std::thread tr1{rb_reader<decltype(rb_r1)>, std::ref(rb_r1)};
    tw1.join();
    tr1.join();

    // attacher of shared memory can act as writer as well.
    SharedMemory shm_c2 = SharedMemory::create_shared_memory("example_shm_rb2", size);
    SharedMemory shm_a2 = SharedMemory::attach_shared_memory("example_shm_rb2");
    CircularBuffer<SharedMemory, uint8_t, NonFlatStructTraits<CharArray, Serializer>> rb_w2{std::move(shm_a2), true};
    CircularBuffer<SharedMemory, uint8_t, NonFlatStructTraits<CharArray, Serializer>> rb_r2{std::move(shm_c2)};

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
