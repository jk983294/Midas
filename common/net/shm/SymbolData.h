#ifndef MIDAS_SYMBOL_DATA_H
#define MIDAS_SYMBOL_DATA_H

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <list>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "midas/md/MdDefs.h"

namespace midas {

static const uint16_t SYMBOL_DATA_INVALID_LOCATOR_CODE = 0xFFFF;
static const uint16_t SYMBOL_DATA_MAX_LOCATOR_CODE = (0xFFFF - 1);

struct __header {
    uint32_t countShm{0};     // already used count
    uint32_t capacityShm{0};  // __data count = 2 * SYMBOL_DATA_MAX_LOCATOR_CODE
};

struct __data {
    __data() { memset(symbol, '\0', sizeof(symbol)); }
    __data(std::string const& symbol, uint16_t locator);
    ~__data() = default;
    __data(__data const&) = default;
    __data& operator=(__data const&) = default;

    char symbol[32 + 1] = {};
    uint16_t locatorShm{SYMBOL_DATA_INVALID_LOCATOR_CODE};
    uint8_t inUse{0};
    uint8_t newSymbol{0};

    struct __vsd {
        std::size_t maxBidDepth = 0;
        std::size_t maxAskDepth = 0;
        uint16_t exchange = 0;
    } vsd[16];
};

inline std::string get_symbol(__data const* d) { return d->symbol; }
inline uint16_t get_locator(__data const* d) { return d->locatorShm; }
inline bool get_in_use(__data const* d) { return d->inUse; }
inline bool get_new_symbol(__data const* d) { return d->newSymbol; }
inline std::size_t get_max_bid_depth(__data::__vsd const* v) { return (v ? v->maxBidDepth : 0); }
inline std::size_t get_max_ask_depth(__data::__vsd const* v) { return (v ? v->maxAskDepth : 0); }
inline uint16_t get_exchange(__data::__vsd const* v) { return (v ? v->exchange : static_cast<uint16_t>(0)); }

inline void set_symbol(__data* d, std::string const& s) {
    memset(d->symbol, '\0', sizeof(d->symbol));
    strncpy(d->symbol, s.c_str(), sizeof(d->symbol) - 1);
}
inline void set_locator(__data* d, uint16_t l) { d->locatorShm = l; }
inline void set_in_use(__data* d, bool dl) { d->inUse = (dl ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0)); }
inline void set_new_symbol(__data* d, bool ns) {
    d->newSymbol = (ns ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0));
}
inline void set_max_bid_depth(__data::__vsd* v, std::size_t n) {
    if (v && (n > v->maxBidDepth)) v->maxBidDepth = n;
}
inline void set_max_ask_depth(__data::__vsd* v, std::size_t n) {
    if (v && (n > v->maxAskDepth)) v->maxAskDepth = n;
}
inline void set_exchange(__data::__vsd* v, uint16_t e) {
    if (v) v->exchange = e;
}

__data::__vsd* get_vsd(__data* d, uint16_t e);
std::ostream& operator<<(std::ostream& o, __data const& d);

class SymbolData {
public:
    enum struct UserFlag : uint8_t { readonly = 0x01, readwrite = 0x02 };

    int fd{-1};
    UserFlag userFlag = UserFlag::readonly;
    void* addr{MAP_FAILED};
    __header* pHeader{nullptr};
    __data* first{nullptr};  // first __data address
    uint16_t maxLocator{SYMBOL_DATA_MAX_LOCATOR_CODE};
    uint32_t symbolCapacity{2 * SYMBOL_DATA_MAX_LOCATOR_CODE};
    std::size_t mappedSize{sizeof(__header) + 2 * SYMBOL_DATA_MAX_LOCATOR_CODE * sizeof(__data)};
    uint16_t generator{0};
    std::size_t pageSize{4096};
    std::unordered_map<std::string, __data*> lookupBySymbol;
    std::unordered_multimap<uint32_t, __data*> lookupByLocator;
    std::list<__data*> pool;
    std::recursive_mutex mutex;

public:
    SymbolData(std::string const& symbolFile, UserFlag flag);

    ~SymbolData() { finish(); }

    SymbolData(SymbolData const&) = delete;

    SymbolData& operator=(SymbolData const&) = delete;

    bool locate(std::string const& symbol, uint16_t& locator, bool create = false);

    bool locate(uint16_t locator, std::vector<__data*>& symbolData);

    bool copy_locate(std::string const& sourceSymbol, std::string const& targetSymbol, uint16_t& locator);

    bool remove(std::string const& symbol);

private:
    void add_symbol(std::string const& symbol, uint16_t& locator, bool assignFreeLocator);
    void add_internal(__data* datum);
    void modify_symbol(std::string const& symbol, uint16_t locator);
    void modify_internal(__data* datum);
    void delete_symbol(std::string const& symbol);
    void delete_internal(__data* datum);
    void clear_internal();
    void mmap(void* address);
    void init();
    void finish();
    void* page_aligned(void* addr) { return (void*)(reinterpret_cast<uint64_t>(addr) & ~(pageSize - 1)); }
};

std::ostream& operator<<(std::ostream& o, SymbolData const& sd);
}

#endif
