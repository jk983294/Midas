#ifndef COMMON_UTILS_NUMBER_HELPER_H
#define COMMON_UTILS_NUMBER_HELPER_H

#include <algorithm>
#include <boost/mpl/if.hpp>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

namespace midas {
// sign/unsigned traits
template <bool>
struct _bool {};

template <typename IntType>
struct _type {};

template <>
struct _type<int8_t> {
    typedef int8_t baseType;
    typedef uint8_t unsignedType;
};
template <>
struct _type<uint8_t> {
    typedef uint8_t baseType;
};
template <>
struct _type<int16_t> {
    typedef int_fast16_t baseType;
    typedef uint_fast16_t unsignedType;
};
template <>
struct _type<uint16_t> {
    typedef uint_fast16_t baseType;
};
template <>
struct _type<int32_t> {
    typedef int32_t baseType;
    typedef uint32_t unsignedType;
};
template <>
struct _type<uint32_t> {
    typedef uint32_t baseType;
};
template <>
struct _type<int64_t> {
    typedef int64_t baseType;
    typedef uint64_t unsignedType;
};
template <>
struct _type<uint64_t> {
    typedef uint64_t baseType;
};

#if __WORDSIZE == 64
template <>
struct _type<long long> {
    typedef long long baseType;
    typedef unsigned long long unsignedType;
};
template <>
struct _type<unsigned long long> {
    typedef unsigned long long baseType;
};
#endif

#define _CONVERT_N(x, y, z) #x #y #z

#define _CONVERT_N2(x, y) \
    _CONVERT_N(x, y, 0)   \
    _CONVERT_N(x, y, 1)   \
    _CONVERT_N(x, y, 2)   \
    _CONVERT_N(x, y, 3)   \
    _CONVERT_N(x, y, 4)   \
    _CONVERT_N(x, y, 5) _CONVERT_N(x, y, 6) _CONVERT_N(x, y, 7) _CONVERT_N(x, y, 8) _CONVERT_N(x, y, 9)

#define _CONVERT_N3(x) \
    _CONVERT_N2(x, 0)  \
    _CONVERT_N2(x, 1)  \
    _CONVERT_N2(x, 2)  \
    _CONVERT_N2(x, 3)  \
    _CONVERT_N2(x, 4) _CONVERT_N2(x, 5) _CONVERT_N2(x, 6) _CONVERT_N2(x, 7) _CONVERT_N2(x, 8) _CONVERT_N2(x, 9)

const char DIGITS[] = _CONVERT_N3(0) _CONVERT_N3(1) _CONVERT_N3(2) _CONVERT_N3(3) _CONVERT_N3(4) _CONVERT_N3(5)
    _CONVERT_N3(6) _CONVERT_N3(7) _CONVERT_N3(8) _CONVERT_N3(9);

// convert 3 digit integer to its string representation
template <typename IntType>
inline const char* int3str(IntType v) {
    return &DIGITS[v * 3];
}

const char INT2_DIGITS[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

template <typename IntType>
inline const char* int2digit_str(IntType v) {
    return &INT2_DIGITS[v * 2];
}

inline int _int_str_size(_bool<false>, uint64_t v) {
    if (v < 10000) {
        if (v < 100)
            return (v < 10) ? 1 : 2;
        else
            return (v < 1000) ? 3 : 4;
    } else if (v < 100000000) {
        if (v < 1000000)
            return (v < 100000) ? 5 : 6;
        else
            return (v < 10000000) ? 7 : 8;
    } else if (v < 1000000000000) {
        if (v < 10000000000)
            return (v < 1000000000) ? 9 : 10;
        else
            return (v < 100000000000) ? 11 : 12;
    } else if (v < 10000000000000000) {
        if (v < 100000000000000)
            return (v < 10000000000000) ? 13 : 14;
        else
            return (v < 1000000000000000) ? 15 : 16;
    } else if (v < 1000000000000000000)
        return (v < 100000000000000000) ? 17 : 18;
    else
        return (v < 10000000000000000000ULL) ? 19 : 20;
}

inline int _int_str_size(_bool<true>, int64_t v) {
    if (v < 0)
        return 1 + _int_str_size(_bool<false>(), static_cast<uint64_t>(-v));
    else
        return _int_str_size(_bool<false>(), static_cast<uint64_t>(v));
}

template <class IntType>
inline int int_str_size(IntType v) {
    return _int_str_size(
        _bool<std::is_signed<IntType>::value>(),
        static_cast<typename std::conditional<std::is_signed<IntType>::value, int64_t, uint64_t>::type>(v));
}
// unsinged version
template <class IntType>
inline int _int2str(_bool<false>, IntType v, char* buffer) {
    int len = _int_str_size(_bool<false>(), static_cast<uint64_t>(v));
    char* c = buffer + len;
    *c-- = '\0';
    const char* tmp;
    for (; v > 999; v /= 1000) {
        tmp = int3str(v % 1000);
        *c-- = tmp[2];
        *c-- = tmp[1];
        *c-- = tmp[0];
    }

    tmp = int3str(v);
    *c-- = tmp[2];
    if (v > 9) {
        *c-- = tmp[1];
        if (v > 99) *c = tmp[0];
    }
    return len;
}

// signed version
template <class IntType>
inline int _int2str(_bool<true>, IntType v, char* buffer) {
    if (v >= 0) {
        return _int2str(_bool<false>(), static_cast<typename _type<IntType>::unsignedType>(v), buffer);
    } else {
        *buffer++ = '-';
        int64_t tmp = v;
        return 1 + _int2str(_bool<false>(), -tmp, buffer);
    }
}

template <>
inline int _int2str(_bool<false>, uint8_t v, char* buffer) {
    const char* tmp = int3str(v);
    if (v > 99) {
        buffer[0] = tmp[0];
        buffer[1] = tmp[1];
        buffer[2] = tmp[2];
        buffer[3] = '\0';
        return 3;
    }
    if (v > 9) {
        buffer[0] = tmp[1];
        buffer[1] = tmp[2];
        buffer[2] = '\0';
        return 2;
    } else {
        buffer[0] = tmp[2];
        buffer[1] = '\0';
        return 1;
    }
}

template <class IntType>
inline int int2str(IntType v, char* buffer) {
    return _int2str(_bool<std::numeric_limits<IntType>::is_signed>(), static_cast<typename _type<IntType>::baseType>(v),
                    buffer);
}

// truncate version
template <class IntType>
inline int int2str(IntType v, char* buffer, size_t size) {
    int len = int_str_size(v);
    if (size > size_t(len)) {
        int2str(v, buffer);
    } else if (size > 0) {
        char tmp[24];
        int2str(v, tmp);
        std::memcpy(buffer, tmp, size - 1);
        buffer[size - 1] = '\0';
    }
    return len;
}

template <class IntType>
inline std::string int2str(IntType v) {
    char tmp[24];
    int len = int2str(v, tmp);
    return std::string(tmp, len);
}

template <class IntType>
inline int _int2str0pad(_bool<false>, IntType v, int width, char* buffer) {
    int len = _int_str_size(_bool<false>(), static_cast<uint64_t>(v));
    while (len < width) {
        *buffer++ = '0';
        ++len;
    }
    _int2str(_bool<false>(), static_cast<uint64_t>(v), buffer);
    return len;
}

template <class IntType>
inline int _int2str0pad(_bool<true>, IntType v, int width, char* buffer) {
    if (v >= 0) {
        return _int2str0pad(_bool<false>(), static_cast<typename _type<IntType>::unsignedType>(v), width, buffer);
    } else {
        *buffer++ = '-';
        int64_t tmp = v;
        return 1 + _int2str0pad(_bool<false>(), -tmp, width - 1, buffer);
    }
}

/**
 * write integer string with output right aligned and padded with zeros
 */
template <class IntType>
inline int int2str0pad(IntType v, int width, char* buffer) {
    return _int2str0pad(_bool<std::numeric_limits<IntType>::is_signed>(),
                        static_cast<typename _type<IntType>::baseType>(v), std::min(width, 20), buffer);
}

template <class IntType>
inline int int2str0pad(IntType v, int width, char* buffer, size_t bufferSize) {
    if (width > 20) width = 20;
    int len = std::max(int_str_size(v), width);
    if (bufferSize > size_t(len)) {
        _int2str0pad(_bool<std::numeric_limits<IntType>::is_signed>(),
                     static_cast<typename _type<IntType>::baseType>(v), width, buffer);
    } else if (bufferSize > 0) {
        char tmp[24];
        _int2str0pad(_bool<std::numeric_limits<IntType>::is_signed>(),
                     static_cast<typename _type<IntType>::baseType>(v), width, tmp);
        std::memcpy(buffer, tmp, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    return len;
}

template <class IntType>
inline std::string int2str0pad(IntType v, int width) {
    char tmp[24];
    int len = int2str0pad(v, width, tmp);
    return std::string(tmp, len);
}
}

#endif
