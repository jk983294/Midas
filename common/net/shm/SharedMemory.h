#ifndef MIDAS_SHARED_MEMORY_H
#define MIDAS_SHARED_MEMORY_H

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <midas/MidasException.h>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace midas {

class SharedMemory {
private:
    struct Meta {
        uint8_t magic[8] = {'M', 'I', 'D', 'A', 'S', 's', 'h', 'm'};
        uint32_t size = 0;
        uint64_t version = 0;
        uint64_t ownerPid = 0;
    };
    static constexpr uint32_t PAGE_SIZE{4096};
    static constexpr uint32_t META_SIZE{PAGE_SIZE};
    static uint32_t _roundup_pagesize(uint32_t x_) { return (x_ + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1)); }

public:
    std::string filename;
    Meta *meta{nullptr};
    uint8_t *buffer{nullptr};
    int fd{-1};
    bool isOwner{false};

public:
    bool is_valid() const { return meta != nullptr; }
    uint8_t *address() const { return reinterpret_cast<uint8_t *>(meta); }
    uint32_t capacity() const { return meta->size; }
    uint32_t map_size() const { return meta->size + META_SIZE; }

    /**
     * Create a SharedMemory
     * @param name_ The shared memory file name
     * @param mapSize The size of the memory, 0 indicates map the entire file. (Attach side option)
     */
    static SharedMemory create_shared_memory(const std::string &name_, uint32_t mapSize) {
        return SharedMemory(name_, mapSize, true);
    }
    /**
     * Attach to a SharedMemory
     * @param name_ The shared memory file name
     */
    static SharedMemory attach_shared_memory(const std::string &name_) { return SharedMemory(name_, 0, false); }
    /**
     * Reclaim ownership to a SharedMemory
     * @param name_ The shared memory file name
     */
    static SharedMemory reclaim_shared_memory(const std::string &name_) { return SharedMemory(name_); }

    ~SharedMemory() { _unmap(); }

    SharedMemory(const SharedMemory &) = delete;
    SharedMemory(SharedMemory &&sm_)
        : filename{sm_.filename}, meta{sm_.meta}, buffer{sm_.buffer}, fd{sm_.fd}, isOwner{sm_.isOwner} {
        sm_.meta = nullptr;
        sm_.buffer = nullptr;
        sm_.fd = -1;
        sm_.isOwner = false;
    }
    SharedMemory &operator=(const SharedMemory &) = delete;
    SharedMemory &operator=(SharedMemory &&rhs_) {
        _unmap();
        filename = rhs_.filename;
        meta = rhs_.meta;
        buffer = rhs_.buffer;
        fd = rhs_.fd;
        isOwner = rhs_.isOwner;
        rhs_.meta = nullptr;
        rhs_.buffer = nullptr;
        rhs_.fd = -1;
        rhs_.isOwner = false;
        return *this;
    }

private:
    uint8_t *_map(uint32_t mapSize, bool create_) {
        int fd = -1;
        mapSize = _roundup_pagesize(mapSize);
        if (create_) {
            fd = shm_open(filename.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
            if (fd < 0) {
                // created under /dev/shm/
                THROW_MIDAS_EXCEPTION(std::string("shm_open: ") << strerror(errno) << " : " << filename);
            }
            if (ftruncate(fd, META_SIZE + mapSize) < 0) {
                close(fd);
                shm_unlink(filename.c_str());
                THROW_MIDAS_EXCEPTION(std::string("ftruncate: ") << strerror(errno) << " : " << filename);
            }
        } else {
            fd = shm_open(filename.c_str(), O_RDWR, 0666);
            if (fd < 0) {
                THROW_MIDAS_EXCEPTION(std::string("shm_open attach: ") + strerror(errno) + " : " + filename);
            }
        }

        struct stat stats;
        if (fstat(fd, &stats) < 0) {
            close(fd);
            if (create_) shm_unlink(filename.c_str());
            THROW_MIDAS_EXCEPTION(std::string("fstat: ") + strerror(errno) + " : " + filename);
        }
        uint32_t size = (mapSize != 0) ? META_SIZE + mapSize : static_cast<uint32_t>(stats.st_size);
        if (static_cast<uint32_t>(stats.st_size) < size) {
            close(fd);
            if (create_) shm_unlink(filename.c_str());
            THROW_MIDAS_EXCEPTION(std::string("map size exceed file size, ") + std::to_string(size) + " > " +
                                  std::to_string(stats.st_size));
        }
        void *addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED) {
            close(fd);
            if (create_) shm_unlink(filename.c_str());
            THROW_MIDAS_EXCEPTION(std::string("mmap: ") + strerror(errno));
        }
        meta = reinterpret_cast<Meta *>(addr);
        buffer = reinterpret_cast<uint8_t *>(addr) + META_SIZE;
        fd = fd;

        Meta dummy;
        if (create_) {
            memset(addr, 0, size);
            dummy.size = mapSize;
            dummy.ownerPid = getpid();
            *meta = dummy;
        } else {
            if (memcmp(meta->magic, &dummy.magic, sizeof(dummy.magic)) != 0) {
                THROW_MIDAS_EXCEPTION(
                    std::string("unexpected magic: " + std::string((char *)meta->magic, sizeof(dummy.magic))));
            }
        }
        return buffer;
    }

    void _unmap() {
        if (fd != -1) {
            munmap(meta, META_SIZE + meta->size);
            close(fd);
            fd = -1;
            if (isOwner) {
                shm_unlink(filename.c_str());
            }
        }
    }
    SharedMemory(const std::string &name_, uint32_t mapSize, bool create_) : filename{name_}, isOwner{create_} {
        if (create_ && mapSize == 0) {
            THROW_MIDAS_EXCEPTION("Cannot create 0-sized shared memory.");
        }
        buffer = _map(mapSize, create_);
    }

    SharedMemory(const std::string &name_) : filename{name_}, isOwner{true} {
        buffer = _map(0, false);
        meta->ownerPid = getpid();
    }
};
}

#endif
