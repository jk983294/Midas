#ifndef MIDAS_SHM_CACHE_H
#define MIDAS_SHM_CACHE_H

#include <errno.h>
#include <fcntl.h>
#include <midas/md/MdExchange.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "SymbolData.h"
#include "midas/md/MdDefs.h"

namespace midas {
class ShmCache {
public:
    std::unique_ptr<SymbolData> symbolData;
    std::string nameBookCache;
    void* addrBookCache{MAP_FAILED};
    std::size_t sizeBookCache{0};
    std::size_t sizeBookMetadata{0};  // = sizeof(BookMetadata) * (exchanges.size() + 1)
    int fdBookCache{-1};
    uint16_t numProducts{0};

public:
    ShmCache(std::string const& cachePropertyName, midas::MdExchange const& exchange, SymbolData* symbolData_);

    ShmCache(ShmCache const&) = delete;

    ShmCache& operator=(ShmCache const&) = delete;

    ~ShmCache();

    std::size_t init_book_cache(uint8_t* startingAddress, midas::MdExchange const& exchange);

    std::string name() const { return nameBookCache; }

    std::size_t size() const { return sizeBookCache; }

    uint8_t* address() const { return (uint8_t*)addrBookCache + sizeBookMetadata; }
};
}

#endif
