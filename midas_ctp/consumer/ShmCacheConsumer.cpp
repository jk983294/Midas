#include <fcntl.h>
#include <midas/md/MdBook.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/log/Log.h>
#include "ShmCacheConsumer.h"

using namespace std;
using namespace midas;

ShmCacheConsumer::ShmCacheConsumer(std::string const& cacheName, std::size_t cacheSize)
    : sizeBookCache(cacheSize), nameBookCache(cacheName) {
    bool rc = true;
    fdBookCache = shm_open(nameBookCache.c_str(), O_RDWR, 0666);
    if (fdBookCache >= 0) {
        if (ftruncate(fdBookCache, sizeBookCache) == 0) {
            struct stat stats;
            if (fstat(fdBookCache, &stats) == 0) {
                std::size_t size = static_cast<size_t>(stats.st_size);
                if (size >= sizeBookCache) {
                    sizeBookCache = size;
                    addrBookCache = mmap(nullptr, sizeBookCache, PROT_READ | PROT_WRITE, MAP_SHARED, fdBookCache, 0);
                    if (addrBookCache != MAP_FAILED) {
                        MIDAS_LOG_INFO("Book cache -- shmName: " << nameBookCache << ", fd: " << fdBookCache
                                                                 << ", size: " << sizeBookCache << ", addr: 0x"
                                                                 << addrBookCache);
                        struct rlimit limit;
                        if (getrlimit(RLIMIT_MEMLOCK, &limit) < 0) {
                            MIDAS_LOG_ERROR("getrlimit: " << errno << " (" << strerror(errno) << ")");
                        } else {
                            MIDAS_LOG_INFO("RLIMIT_MEMLOCK - soft_limit: " << limit.rlim_cur
                                                                           << ", hard_limit: " << limit.rlim_max);
                        }
                        if (mlock(addrBookCache, sizeBookCache) < 0) {
                            MIDAS_LOG_ERROR("mlock: " << errno << " (" << strerror(errno) << ")");
                        }
                    } else {
                        MIDAS_LOG_ERROR("mmap: " << errno << " (" << strerror(errno) << ")");
                        rc = false;
                    }
                } else {
                    rc = false;
                }
            } else {
                MIDAS_LOG_ERROR("fstat: " << errno << " (" << strerror(errno) << ")");
                rc = false;
            }
        } else {
            MIDAS_LOG_ERROR("ftruncate: " << errno << " (" << strerror(errno) << ")");
            rc = false;
        }
    } else {
        MIDAS_LOG_ERROR("shm_open: " << errno << " (" << strerror(errno) << ")");
        rc = false;
    }

    if (!rc) {
        MIDAS_LOG_ERROR("Failed to attach to book cache - shmName: " << nameBookCache << ", fd: " << fdBookCache
                                                                     << ", size: " << sizeBookCache);
        close(fdBookCache);
        fdBookCache = -1;
    } else {
        uint16_t d = 0;
        std::size_t metadataBytes = 0;
        for (BookMetadata* pMeta = (BookMetadata*)addrBookCache; pMeta->exchange != ExchangeNone; ++pMeta) {
            d = static_cast<uint16_t>(d + pMeta->exchangeDepth);  // levels will not exceed size!
            metadataBytes += sizeof(BookMetadata);
        }
        metadataBytes += sizeof(BookMetadata);

        addrPostMetadata = (uint8_t*)addrBookCache + metadataBytes;
        shmBytesPerProduct = bytes_per_product(d);
        shmBytesOffsetBid = 0;
        shmBytesOffsetAsk = bytes_offset_ask(d);
        MIDAS_LOG_INFO("shmBytesPerProduct: " << shmBytesPerProduct << ", shmBytesOffsetBid: " << shmBytesOffsetBid
                                              << ", shmBytesOffsetAsk: " << shmBytesOffsetAsk
                                              << ", addrPostMetadata: " << metadataBytes);
    }
}

ShmCacheConsumer::~ShmCacheConsumer() {
    MIDAS_LOG_INFO("Destroying shared memory book cache " << nameBookCache);
    if (addrBookCache != MAP_FAILED) {
        munmap(addrBookCache, sizeBookCache);
    }
    if (fdBookCache >= 0) {
        close(fdBookCache);
    }
}
