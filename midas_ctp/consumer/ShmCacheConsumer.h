#ifndef MIDAS_SHM_CACHE_CONSUMER_H
#define MIDAS_SHM_CACHE_CONSUMER_H

#include <cstddef>
#include <string>

#include <stdint.h>
#include <sys/mman.h>

class ShmCacheConsumer {
public:
    std::size_t sizeBookCache;
    std::string nameBookCache;
    int fdBookCache{-1};
    void* addrBookCache{MAP_FAILED};
    uint8_t* addrPostMetadata{nullptr};
    uint16_t shmBytesPerProduct{0};
    uint16_t shmBytesOffsetBid{0};
    uint16_t shmBytesOffsetAsk{0};

public:
    ShmCacheConsumer(std::string const& cacheName, std::size_t cacheSize);

    ~ShmCacheConsumer();

    std::string name() const { return nameBookCache; }

    std::size_t size() const { return sizeBookCache; }

    uint8_t* startAddress() const { return (uint8_t*)addrBookCache; }

    uint8_t* postMetadataAddress() const { return addrPostMetadata; }

    uint16_t bytesPerProduct() const { return shmBytesPerProduct; }

    uint16_t bytesOffsetBid() const { return shmBytesOffsetBid; }

    uint16_t bytesOffsetAsk() const { return shmBytesOffsetAsk; }
};

#endif
