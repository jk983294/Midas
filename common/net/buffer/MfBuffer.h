#ifndef MIDAS_MF_BUFFER_H
#define MIDAS_MF_BUFFER_H

#include <algorithm>
#include <boost/utility/string_ref.hpp>
#include <cassert>
#include <cstring>
#include <ctime>
#include <limits>
#include <memory>
#include <string>
#include "midas/MidasConstants.h"
#include "net/buffer/BufferDataAllocator.h"
#include "net/buffer/ConstBuffer.h"
#include "net/common/MidasHeader.h"
#include "utils/ConvertHelper.h"

namespace midas {

template <typename Allocator>
class TMfBuffer {
public:
    typedef typename Allocator::value_type buffer_value_type;
    typedef typename Allocator::size_type buffer_size_type;
    typedef typename Allocator::pointer buffer_pointer;
    typedef typename Allocator::const_pointer buffer_const_pointer;

    Allocator alloc;
    buffer_size_type minCapacity;
    buffer_pointer pBuffer{nullptr};
    buffer_size_type bufferSize{0};
    buffer_size_type bufferUsed{0};
    buffer_size_type bufferUsedStart{0};

public:
    TMfBuffer(const Allocator& alloc_ = Allocator(), buffer_size_type minCapacity_ = 128)
        : alloc(alloc_), minCapacity(minCapacity_) {}

    virtual ~TMfBuffer() {
        if (pBuffer) {
            alloc.deallocate(pBuffer, bufferSize);
        }
    }

    TMfBuffer(const TMfBuffer<Allocator>& rhs)
        : alloc(rhs.alloc),
          minCapacity(rhs.minCapacity),
          pBuffer(alloc.allocate(rhs.bufferSize, rhs.buffer)),
          bufferSize(rhs.bufferSize),
          bufferUsed(rhs.bufferUsed),
          bufferUsedStart(rhs.bufferUsedStart) {
        std::memcpy(pBuffer, rhs.pBuffer, bufferUsed);
    }

    void swap(TMfBuffer<Allocator>& rhs) {
        std::swap(alloc, rhs.alloc);
        std::swap(minCapacity, rhs.minCapacity);
        std::swap(pBuffer, rhs.pBuffer);
        std::swap(bufferSize, rhs.bufferSize);
        std::swap(bufferUsed, rhs.bufferUsed);
        std::swap(bufferUsedStart, rhs.bufferUsedStart);
    }

    TMfBuffer<Allocator>& operator=(TMfBuffer<Allocator> rhs) {
        this->swap(rhs);
        return *this;
    }

    void reserve(buffer_size_type minCapacity_) {
        if (!pBuffer) {
            bufferSize = std::max(minCapacity, minCapacity_);
            pBuffer = alloc.allocate(bufferSize);
        } else if (minCapacity_ > bufferSize) {
            // enlarge by double it
            buffer_size_type newSize = std::max(minCapacity_, bufferSize * 2);
            buffer_pointer newBuffer = alloc.allocate(newSize, pBuffer);
            std::memcpy(newBuffer, pBuffer, bufferUsed);
            alloc.deallocate(pBuffer, bufferSize);
            pBuffer = newBuffer;
            bufferSize = newSize;
        }
    }

    buffer_size_type capacity() const { return bufferSize; }

    /**
     * header size + message size
     */
    buffer_size_type size() const { return bufferUsed; }

    buffer_const_pointer data() const { return pBuffer; }

    /**
     * no header, payload only
     */
    string mf_string() const {
        if (!pBuffer || bufferUsed < buffer_size_type(sizeof(MidasHeader))) {
            return string();
        } else {
            return string(pBuffer + sizeof(MidasHeader), bufferUsed - sizeof(MidasHeader));
        }
    }

    /**
     * header + all buffer contents
     */
    string full_string() const {
        if (!pBuffer) {
            return string();
        } else {
            return string(pBuffer, bufferUsed);
        }
    }

    void start(const boost::string_ref& id) {
        buffer_size_type need = id.size() + 5;
        reserve(sizeof(MidasHeader) + need);
        buffer_pointer p = pBuffer + sizeof(MidasHeader);
        bufferUsed = sizeof(MidasHeader) + need;
        bufferUsedStart = bufferUsed;
        *p++ = 'i';
        *p++ = 'd';
        *p++ = ' ';
        std::memcpy(p, id.begin(), id.size());
        p += id.size();
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * keep id part, let the rest be reset
     */
    void restart() { bufferUsed = bufferUsedStart; }

    /**
     * should not contain un-support character
     */
    void add_string(const boost::string_ref& key, const char* value, size_t valueLen) {
        buffer_size_type need = key.size() + 2;
        if (valueLen > 0) need += valueLen + 1;
        reserve(bufferUsed + need);  // make sure big enough

        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += need;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';

        if (valueLen > 0) {
            std::memcpy(p, value, valueLen);
            p += valueLen;
            *p++ = ' ';
        }
        *p++ = ',';
    }

    void add_string(const boost::string_ref& key, const boost::string_ref& value) {
        add_string(key, value.begin(), value.size());
    }

    /**
     * convert un-support character to underscore
     */
    void add_encoded_string(const boost::string_ref& key, const char* value, size_t valueLen) {
        valueLen = strnlen(value, valueLen);  // terminate at first null and remove tailing space
        while (valueLen > 0 && value[valueLen - 1] == ' ') {
            --valueLen;
        }

        buffer_size_type needed = key.size() + 3;
        if (valueLen > 0) needed += valueLen + 1;

        reserve(bufferUsed + needed);

        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';

        if (valueLen > 0) {
            const char* end = value + valueLen;
            for (; value != end; ++value) {
                switch (*value) {
                    case ' ':
                    case ';':
                    case ',':
                        *p++ = '_';
                        break;
                    default:
                        *p++ = *value;
                        break;
                }
            }
            *p++ = ' ';
        }
        *p++ = ',';
    }

    void add_encoded_string(const boost::string_ref& key, const boost::string_ref& value) {
        add_encoded_string(key, value.begin(), value.size());
    }

    template <typename IntType>
    void add_int(const boost::string_ref& key, IntType value) {
        buffer_size_type need = key.size() + int_str_size(value) + 3;
        reserve(bufferUsed + need);  // make sure big enough

        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += need;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';

        p += int2str(value, p);
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * precision of 6 digits with trailing 0 removed
     * @tparam FloatType
     * @param key
     * @param value
     */
    template <typename FloatType>
    void add_float(const boost::string_ref& key, FloatType value) {
        add_float(key, value, 6);
    }

    template <typename FloatType>
    void add_float(const boost::string_ref& key, FloatType value, int precision) {
        char str[64];
        size_t len = float_2_string_trim(value, precision, str, sizeof(str));
        buffer_size_type needed = key.size() + len + 3;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        std::memcpy(p, str, len);
        p += len;
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * places number of digits after decimal point
     * @tparam IntType
     * @param key
     * @param value
     * @param places
     */
    template <typename IntType>
    void add_fixed_point(const boost::string_ref& key, IntType value, int places) {
        buffer_size_type needed = key.size() + fixed_point_string_size(value, places) + 3;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        p += fixed_point_2_string(value, places, p);
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * places number of digits after decimal point with trailing 0 removed
     * @tparam IntType
     * @param key
     * @param value
     * @param places
     */
    template <typename IntType>
    void add_fixed_point_trim(const boost::string_ref& key, IntType value, int places) {
        buffer_size_type needed = key.size() + fixed_point_string_size(value, places) + 3;  // max size if no trim
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        p += fixed_point_2_string_trim(value, places, p);
        *p++ = ' ';
        *p++ = ',';
        bufferUsed = p - pBuffer;
    }

    void add_timestamp(const boost::string_ref& key, time_t second, int32_t usecond) {
        buffer_size_type needed = key.size() + int_str_size(second) + 10;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        p += int2str(second, p);
        *p++ = '.';
        p += int2str0pad(usecond, 6, p);
        *p++ = ' ';
        *p++ = ',';
    }

    void add_microsecond_intraday(const boost::string_ref& key, unsigned microsecond) {
        char tmp[16];
        int len = microsecond_2_string_intraday(microsecond, tmp);
        add_string(key, tmp, len);
    }

    /**
     * "HHMM" format
     */
    void add_local_time_HM(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        localtime_r(&t, &tmp);
        add_time_HM(key, tmp.tm_hour, tmp.tm_min);
    }

    void add_utc_time_HM(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        gmtime_r(&t, &tmp);
        add_time_HM(key, tmp.tm_hour, tmp.tm_min);
    }

    void add_time_HM(const boost::string_ref& key, int hour, int minute) {
        buffer_size_type needed = key.size() + 7;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        const char* tmp = int2digit_str(hour);
        *p++ = tmp[0];
        *p++ = tmp[1];
        tmp = int2digit_str(minute);
        *p++ = tmp[0];
        *p++ = tmp[1];
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * "HHMMSS" format
     */
    void add_local_time_HMS(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        localtime_r(&t, &tmp);
        add_time_HMS(key, tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
    }

    void add_utc_time_HMS(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        gmtime_r(&t, &tmp);
        add_time_HMS(key, tmp.tm_hour, tmp.tm_min, tmp.tm_sec);
    }

    void add_time_HMS(const boost::string_ref& key, int hour, int minute, int second) {
        buffer_size_type needed = key.size() + 9;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        const char* tmp = int2digit_str(hour);
        *p++ = tmp[0];
        *p++ = tmp[1];
        tmp = int2digit_str(minute);
        *p++ = tmp[0];
        *p++ = tmp[1];
        tmp = int2digit_str(second);
        *p++ = tmp[0];
        *p++ = tmp[1];
        *p++ = ' ';
        *p++ = ',';
    }

    /**
     * "YYYYMMDD" format
     */
    void add_local_date(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        localtime_r(&t, &tmp);
        add_date(key, tmp.tm_year + 1900, tmp.tm_mon + 1, tmp.tm_mday);
    }

    void add_utc_date(const boost::string_ref& key, time_t t = std::time(0)) {
        std::tm tmp;
        gmtime_r(&t, &tmp);
        add_date(key, tmp.tm_year + 1900, tmp.tm_mon + 1, tmp.tm_mday);
    }

    void add_date(const boost::string_ref& key, int year, int month, int day) {
        add_int(key, (year * 10000) + (month * 100) + day);
    }

    template <typename IntType>
    void add_date(const boost::string_ref& key, IntType value) {
        char tmp[16];
        int i = int2str(value, tmp);
        add_string(key, tmp, i);
    }

    /**
     * key with no value
     */
    void add(const boost::string_ref& key) {
        buffer_size_type needed = key.size() + 2;
        reserve(bufferUsed + needed);
        buffer_pointer p = pBuffer + bufferUsed;
        bufferUsed += needed;
        std::memcpy(p, key.begin(), key.size());
        p += key.size();
        *p++ = ' ';
        *p++ = ',';
    }

    template <typename IntType>
    void add_price(const boost::string_ref& key, IntType value, int priceScale) {
        add_fixed_point_trim(key, value, priceScale);
    }

    void finish(time_t t = std::time(0), uint32_t seq = 0, uint8_t group = 1) {
        reserve(bufferUsed + 1);
        *(pBuffer + bufferUsed++) = ';';
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        write_midas_header(pBuffer, bufferUsed - sizeof(MidasHeader), ts.tv_sec, (ts.tv_nsec / 1000), group, seq);
    }
};

struct MfBufferPoolTag {};
static const size_t fastSize1 = BufferSize<128>::size;
typedef FastPoolAllocator<fastSize1, MfBufferPoolTag> PoolAlloc1;
static const size_t fastSize2 = BufferSize<256>::size;
typedef FastPoolAllocator<fastSize2, MfBufferPoolTag> PoolAlloc2;
static const size_t fastSize3 = BufferSize<512>::size;
typedef FastPoolAllocator<fastSize3, MfBufferPoolTag> PoolAlloc3;

template <typename AllocT1 = PoolAlloc1, typename AllocT2 = PoolAlloc2, typename AllocT3 = PoolAlloc3>
class MfBufferAlloc : public TMfBuffer<Allocator<AllocT1, AllocT2, AllocT3>> {
public:
    typedef Allocator<AllocT1, AllocT2, AllocT3> TAllocator;
    typedef TMfBuffer<TAllocator> MfBufferBase;

public:
    MfBufferAlloc() : MfBufferBase(TAllocator(), fastSize1) {}

    // take ownership of data
    ConstBuffer buffer() {
        BufferDataPtr ptr(reinterpret_cast<BufferData*>(this->pBuffer));
        ptr->prefix().size = this->size();

        this->alloc.deallocate(this->pBuffer, this->bufferSize);
        this->pBuffer = typename MfBufferBase::buffer_pointer(nullptr);
        this->bufferSize = 0;
        this->bufferUsed = 0;

        return ConstBuffer(ptr);
    }
};

typedef MfBufferAlloc<> MfBuffer;
}

#endif
