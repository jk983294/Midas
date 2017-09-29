#ifndef MIDAS_FLOAT_HELPER_H
#define MIDAS_FLOAT_HELPER_H

#include "MidasNumberUtils.h"

using namespace std;

namespace midas {
static const int MAX_PLACES = 20;
static const char NAN_STR[] = "nan";
static const char INF_STR[] = "inf";

// for unsigned
inline int _fixed_point_string_size(_bool<false>, uint64_t v, int places) {
    return std::max(_int_str_size(_bool<false>(), static_cast<uint64_t>(v)) + 1, places + 2);
}

inline int _fixed_point_string_size(_bool<true>, int64_t v, int places) {
    if (v < 0) {
        return 1 + _fixed_point_string_size(_bool<false>(), static_cast<uint64_t>(-v), places);
    } else {
        return _fixed_point_string_size(_bool<false>(), static_cast<uint64_t>(v), places);
    }
}

/**
 * string length for this fixed point value
 * @param v input integer
 * @param places number of digits after decimal point
 */
template <typename IntType>
inline int fixed_point_string_size(IntType v, int places) {
    if (places <= 0)
        return int_str_size(v);
    else if (places > MAX_PLACES)
        places = MAX_PLACES;
    return _fixed_point_string_size(_bool<std::numeric_limits<IntType>::is_signed>(), v, places);
}

template <typename IntType>
inline int _fixed_point_2_string(_bool<false>, IntType v, int places, char* p) {
    char tmp[24];
    int len = _int2str(_bool<false>(), v, tmp);
    if (len <= places) {
        *p++ = '0';
        *p++ = '.';
        for (int i = places - len; i != 0; --i) {
            *p++ = '0';
        }
        const char* ch = tmp;
        const char* chEnd = tmp + len;
        do {
            *p++ = *ch++;
        } while (ch != chEnd);
        *p++ = '\0';
        return places + 2;
    } else {
        const char* ch = tmp;
        const char* chEnd = tmp + len - places;
        do {
            *p++ = *ch++;
        } while (ch != chEnd);
        *p++ = '.';
        chEnd = tmp + len;
        do {
            *p++ = *ch++;
        } while (ch != chEnd);
        *p++ = '\0';
        return len + 1;
    }
}

template <typename IntType>
inline int _fixed_point_2_string(_bool<true>, IntType v, int places, char* p) {
    if (v < 0) {
        *p++ = '-';
        int64_t tmp = v;
        return 1 + _fixed_point_2_string(_bool<false>(), -tmp, places, p);
    } else {
        return _fixed_point_2_string(_bool<false>(), static_cast<typename _type<IntType>::unsignedType>(v), places, p);
    }
}

template <typename IntType>
inline int fixed_point_2_string(IntType v, int places, char* p) {
    if (places <= 0)
        return int2str(v, p);
    else if (places > MAX_PLACES)
        places = MAX_PLACES;

    return _fixed_point_2_string(_bool<std::numeric_limits<IntType>::is_signed>(),
                                 static_cast<typename _type<IntType>::baseType>(v), places, p);
}

template <typename IntType>
inline int fixed_point_2_string(IntType v, int places, char* p, size_t bufferSize) {
    if (places <= 0)
        return int2str(v, p);
    else if (places > MAX_PLACES)
        places = MAX_PLACES;

    int len = _fixed_point_string_size(_bool<std::numeric_limits<IntType>::is_signed>(), v, places);
    if (bufferSize > size_t(len)) {
        _fixed_point_2_string(_bool<std::numeric_limits<IntType>::is_signed>(),
                              static_cast<typename _type<IntType>::baseType>(v), places, p);
    } else if (bufferSize > 0) {
        char tmp[24];
        _fixed_point_2_string(_bool<std::numeric_limits<IntType>::is_signed>(),
                              static_cast<typename _type<IntType>::baseType>(v), places, tmp);
        std::memcpy(p, tmp, bufferSize - 1);
        p[bufferSize - 1] = '\0';
    }
    return len;
}

template <typename IntType>
inline string fixed_point_2_string(IntType v, int places) {
    char tmp[24];
    int len = fixed_point_2_string(v, places, tmp);
    return string(tmp, len);
}

/**
 * all trailing zeros removed
 * decimal point will get removed if all digits after decimal point were zero
 */
template <typename IntType>
inline int fixed_point_2_string_trim(IntType v, int places, char* p) {
    while (places > 0 && !(v % 10)) {
        --places;
        v /= 10;
    }
    return fixed_point_2_string(v, places, p);
}

template <typename IntType>
inline int fixed_point_2_string_trim(IntType v, int places, char* p, size_t bufferSize) {
    while (places > 0 && !(v % 10)) {
        --places;
        v /= 10;
    }
    return fixed_point_2_string(v, places, p, bufferSize);
}

template <typename IntType>
inline string fixed_point_2_string_trim(IntType v, int places) {
    char tmp[24];
    int len = fixed_point_2_string_trim(v, places, tmp);
    return string(tmp, len);
}

template <typename FloatType>
struct FloatFormat {};

template <>
struct FloatFormat<double> {
    static const char* format_0f() { return "%.0f"; }
    static const char* format_xf() { return "%.*f"; }
};

template <>
struct FloatFormat<long double> {
    static const char* format_0f() { return "%.0Lf"; }
    static const char* format_xf() { return "%.*Lf"; }
};

static const uint64_t POWER_OF_TEN[] = {
    1,         10,         100,         1000,         10000,         100000,         1000000,         10000000,
    100000000, 1000000000, 10000000000, 100000000000, 1000000000000, 10000000000000, 100000000000000, 1000000000000000};

static const double SCALE_FACTOR[] = {0.000000000000001,
                                      0.00000000000001,
                                      0.0000000000001,
                                      0.000000000001,
                                      0.00000000001,
                                      0.0000000001,
                                      0.000000001,
                                      0.00000001,
                                      0.0000001,
                                      0.000001,
                                      0.00001,
                                      0.0001,
                                      0.001,
                                      0.01,
                                      0.1,
                                      1,
                                      10,
                                      100,
                                      1000,
                                      10000,
                                      100000,
                                      1000000,
                                      10000000,
                                      100000000,
                                      1000000000,
                                      10000000000,
                                      100000000000,
                                      1000000000000,
                                      10000000000000,
                                      100000000000000,
                                      1000000000000000};

static const int MaxScaleFactorIndex = 15;
static const double* MiddleScale = &SCALE_FACTOR[MaxScaleFactorIndex];

template <typename FloatType, bool TRIM>
inline int _float_2_string_core(FloatType v, int precision, char* p) {
    char* ch = p;
    if (v < 0) {
        v = -v;
        *ch++ = '-';
    }

    uint64_t integerPart = static_cast<uint64_t>(v);
    uint64_t decimalPart = 0;
    if (precision > 0) {
        FloatType decimal = (v - integerPart) * POWER_OF_TEN[precision];
        decimalPart = static_cast<uint64_t>(decimal);
        FloatType diff = decimal - decimalPart;
        if (diff > 0.5) {
            ++decimalPart;
            if (decimalPart == POWER_OF_TEN[precision]) {
                decimalPart = 0;
                ++integerPart;
            }
        } else if ((diff == 0.5) && ((decimalPart == 0) || (decimalPart & 1))) {
            // round half to even
            ++decimalPart;
        }
    } else {
        FloatType diff = v - integerPart;
        if ((diff > 0.5) || ((diff == 0.5) && (integerPart & 1))) {
            // round half to even
            ++integerPart;
        }
    }

    ch += _int2str(_bool<false>(), integerPart, ch);
    if (precision > 0 && (!TRIM || decimalPart > 0)) {
        *ch++ = '.';
        ch += _int2str0pad(_bool<false>(), decimalPart, precision, ch);
        if (TRIM && *(ch - 1) == '0') {
            do {
                --ch;
            } while (*(ch - 1) == '0');
            *ch = '\0';
        }
    }
    return ch - p;
}

template <typename FloatType, bool TRIM>
inline int _float_2_string(FloatType v, int precision, char* p, size_t bufferSize) {
    if (isnan(v)) {
        if (bufferSize >= sizeof(NAN_STR)) {
            memcpy(p, NAN_STR, sizeof(NAN_STR));
        } else if (bufferSize > 0) {
            size_t len = std::min(bufferSize, sizeof(NAN_STR)) - 1;
            memcpy(p, NAN_STR, len);
            p[len] = '\0';
        }
        return sizeof(NAN_STR) - 1;
    } else if (isinf(v)) {
        if (bufferSize >= sizeof(INF_STR)) {
            memcpy(p, INF_STR, sizeof(INF_STR));
        } else if (bufferSize > 0) {
            size_t len = std::min(bufferSize, sizeof(INF_STR)) - 1;
            memcpy(p, INF_STR, len);
            p[len] = '\0';
        }
        return sizeof(INF_STR) - 1;
    }

    int pre = std::min(max(precision, 0), 15);
    if ((v > FloatType(std::numeric_limits<int64_t>::max())) || (v < FloatType(std::numeric_limits<int64_t>::min()))) {
        if (TRIM) {
            return std::snprintf(p, bufferSize, FloatFormat<FloatType>::format_0f(), v);
        } else {
            return std::snprintf(p, bufferSize, FloatFormat<FloatType>::format_xf(), pre, v);
        }
    } else if (bufferSize < size_t(pre + 22)) {
        char tmp[40];
        int len = _float_2_string_core<FloatType, TRIM>(v, pre, tmp);
        if (bufferSize > 0) {
            memcpy(p, tmp, bufferSize - 1);
            p[bufferSize - 1] = '\0';
        }
        return len;
    } else {
        return _float_2_string_core<FloatType, TRIM>(v, pre, p);
    }
}

template <typename FloatType>
inline int float_2_string(FloatType v, int precision, char* p, size_t bufferSize) {
    return _float_2_string<double, false>(v, precision, p, bufferSize);
}

template <>
inline int float_2_string(long double v, int precision, char* p, size_t bufferSize) {
    return _float_2_string<long double, false>(v, precision, p, bufferSize);
}

template <typename FloatType>
inline string float_2_string(FloatType v, int precision) {
    char tmp[312];
    int len = _float_2_string<double, false>(v, precision, tmp, sizeof(tmp));
    return string(tmp, len);
}

template <>
inline string float_2_string(long double v, int precision) {
    char tmp[360];
    int len = _float_2_string<long double, false>(v, precision, tmp, sizeof(tmp));
    return string(tmp, len);
}

template <typename FloatType>
inline int float_2_string_trim(FloatType v, int precision, char* p, size_t bufferSize) {
    return _float_2_string<double, true>(v, precision, p, bufferSize);
}

template <>
inline int float_2_string_trim(long double v, int precision, char* p, size_t bufferSize) {
    return _float_2_string<long double, true>(v, precision, p, bufferSize);
}

template <typename FloatType>
inline string float_2_string_trim(FloatType v, int precision) {
    char tmp[312];
    int len = _float_2_string<double, true>(v, precision, tmp, sizeof(tmp));
    return string(tmp, len);
}

template <>
inline string float_2_string_trim(long double v, int precision) {
    char tmp[360];
    int len = _float_2_string<long double, true>(v, precision, tmp, sizeof(tmp));
    return string(tmp, len);
}

template <typename IntType>
inline IntType fixed_point_normalize(IntType v, uint8_t scaleFactor, uint8_t newScaleFactor) {
    double multiply = *(MiddleScale + static_cast<int8_t>(newScaleFactor - scaleFactor));
    return static_cast<IntType>(multiply * v);
}

template <typename IntType>
inline double fixed_point_2_double(IntType v, uint8_t scaleFactor) {
    double multiply = *(MiddleScale - static_cast<int8_t>(scaleFactor));
    return multiply * v;
}

inline uint64_t _scale_up_down(uint64_t v, int8_t scaleFactor) {
    switch (scaleFactor) {
        case -15:
            return v / 1000000000000000ULL;
        case -14:
            return v / 100000000000000ULL;
        case -13:
            return v / 10000000000000ULL;
        case -12:
            return v / 1000000000000ULL;
        case -11:
            return v / 100000000000ULL;
        case -10:
            return v / 10000000000ULL;
        case -9:
            return v / 1000000000ULL;
        case -8:
            return v / 100000000ULL;
        case -7:
            return v / 10000000ULL;
        case -6:
            return v / 1000000ULL;
        case -5:
            return v / 100000ULL;
        case -4:
            return v / 10000ULL;
        case -3:
            return v / 1000ULL;
        case -2:
            return v / 100ULL;
        case -1:
            return v / 10ULL;
        case 0:
            return v;
        case 1:
            return v * 10ULL;
        case 2:
            return v * 100ULL;
        case 3:
            return v * 1000ULL;
        case 4:
            return v * 10000ULL;
        case 5:
            return v * 100000ULL;
        case 6:
            return v * 1000000ULL;
        case 7:
            return v * 10000000ULL;
        case 8:
            return v * 100000000ULL;
        case 9:
            return v * 1000000000ULL;
        case 10:
            return v * 10000000000ULL;
        case 11:
            return v * 100000000000ULL;
        case 12:
            return v * 1000000000000ULL;
        case 13:
            return v * 10000000000000ULL;
        case 14:
            return v * 100000000000000ULL;
        case 15:
            return v * 1000000000000000ULL;
        default:
            abort();
    }
}

static const int64_t _int_scale_up_max[] = {LLONG_MAX,
                                            LLONG_MAX / 10LL,
                                            LLONG_MAX / 100LL,
                                            LLONG_MAX / 1000LL,
                                            LLONG_MAX / 10000LL,
                                            LLONG_MAX / 100000LL,
                                            LLONG_MAX / 1000000LL,
                                            LLONG_MAX / 10000000LL,
                                            LLONG_MAX / 100000000LL,
                                            LLONG_MAX / 1000000000LL,
                                            LLONG_MAX / 10000000000LL,
                                            LLONG_MAX / 100000000000LL,
                                            LLONG_MAX / 1000000000000LL,
                                            LLONG_MAX / 10000000000000LL,
                                            LLONG_MAX / 100000000000000LL,
                                            LLONG_MAX / 1000000000000000LL};

static const uint64_t _uint_scale_up_max[] = {ULLONG_MAX,
                                              ULLONG_MAX / 10ULL,
                                              ULLONG_MAX / 100ULL,
                                              ULLONG_MAX / 1000ULL,
                                              ULLONG_MAX / 10000ULL,
                                              ULLONG_MAX / 100000ULL,
                                              ULLONG_MAX / 1000000ULL,
                                              ULLONG_MAX / 10000000ULL,
                                              ULLONG_MAX / 100000000ULL,
                                              ULLONG_MAX / 1000000000ULL,
                                              ULLONG_MAX / 10000000000ULL,
                                              ULLONG_MAX / 100000000000ULL,
                                              ULLONG_MAX / 1000000000000ULL,
                                              ULLONG_MAX / 10000000000000ULL,
                                              ULLONG_MAX / 100000000000000ULL,
                                              ULLONG_MAX / 1000000000000000ULL};

template <typename ValueType>
inline int64_t str2int64(const ValueType& v, int8_t scaleFactor, int* rc = nullptr) {
    if (15 < scaleFactor || -15 > scaleFactor) {
        if (rc) *rc = ERANGE;
        return 0;
    }

    auto begin = v.begin();
    auto end = v.end();
    auto it = std::find_if_not(begin, end, [](char ch) { return isspace(ch); });
    bool isNegative = false;
    if (end != it) {
        if ('+' == *it)
            ++it;
        else if ('-' == *it) {
            isNegative = true;
            ++it;
        }
    }

    char buffer[24];
    size_t index = 0;
    int8_t* scaleUpFactor = nullptr;
    std::find_if(it, end, [&](char ch) {
        if (sizeof(buffer) > index + 1) {
            if ('0' <= ch && '9' >= ch) {
                buffer[index] = ch;
                ++index;
                return scaleUpFactor ? 0 == --(*scaleUpFactor) : false;
            }

            if ('.' == ch && 0 < scaleFactor && !scaleUpFactor) {
                scaleUpFactor = &scaleFactor;
                return false;
            }
        }
        return true;
    });
    buffer[index] = '\0';

    errno = 0;
    char* endPtr = nullptr;
    int64_t result = strtoll(buffer, &endPtr, 10);
    if (0 != errno) {
        if (rc) *rc = errno;
        errno = 0;
        if (isNegative && LLONG_MAX == result) return LLONG_MIN;
        return result;
    }

    auto sf = scaleUpFactor ? *scaleUpFactor : scaleFactor;
    if (0 < sf && _int_scale_up_max[sf] < result) {
        if (rc) *rc = ERANGE;
        return isNegative ? LLONG_MIN : LLONG_MAX;
    }
    result = _scale_up_down(result, sf);
    return isNegative ? -result : result;
}

template <typename ValueType>
inline uint64_t str2uint64(const ValueType& v, int8_t scaleFactor, int* rc = nullptr) {
    if (15 < scaleFactor || -15 > scaleFactor) {
        if (rc) *rc = ERANGE;
        return 0;
    }

    auto begin = v.begin();
    auto end = v.end();
    auto it = std::find_if_not(begin, end, [](char ch) { return isspace(ch); });
    if (end != it && '+' == *it) {
        ++it;
    }

    char buffer[24];
    size_t index = 0;
    int8_t* scaleUpFactor = nullptr;
    std::find_if(it, end, [&](char ch) {
        if (sizeof(buffer) > index + 1) {
            if ('0' <= ch && '9' >= ch) {
                buffer[index] = ch;
                ++index;
                return scaleUpFactor ? 0 == --(*scaleUpFactor) : false;
            }

            if ('.' == ch && 0 < scaleFactor && !scaleUpFactor) {
                scaleUpFactor = &scaleFactor;
                return false;
            }
        }
        return true;
    });
    buffer[index] = '\0';

    errno = 0;
    char* endPtr = nullptr;
    uint64_t result = strtoull(buffer, &endPtr, 10);
    if (0 != errno) {
        if (rc) *rc = errno;
        errno = 0;
        return result;
    }

    auto sf = scaleUpFactor ? *scaleUpFactor : scaleFactor;
    if (0 < sf && _uint_scale_up_max[sf] < result) {
        if (rc) *rc = ERANGE;
        return ULLONG_MAX;
    }
    return _scale_up_down(result, sf);
}

/**
 * 1e6 => 1000000
 * @param num
 * @return
 */
inline string expand_scientific_number(const string& num) {
    string result;
    const auto ePos = num.find_first_of("Ee");

    if (ePos != string::npos) {
        const int exponent = atoi(num.c_str() + ePos + 1);
        const auto decimalPos = num.find('.');
        if (exponent >= 0) {
            if (decimalPos == string::npos) {  // 12e3
                result.reserve(ePos + exponent + 1);
                result = num.substr(0, ePos);
                result.append(exponent, '0');
            } else {
                const int leadingZero = (*num.c_str() == '0') && (exponent != 0);
                const int remainingDigits = ePos - decimalPos - 1;
                const int extraZeros = exponent - remainingDigits;
                if (extraZeros >= 0) {  // 1.2 e3
                    result.reserve(decimalPos - leadingZero + remainingDigits + extraZeros + 1);
                    result = num.substr(leadingZero, decimalPos - leadingZero);
                    result.append(num.substr(decimalPos + 1, remainingDigits));
                    result.append(extraZeros, '0');
                } else {  // 1.23e1
                    result.reserve(leadingZero + ePos + 1);
                    result = num.substr(leadingZero, decimalPos - leadingZero);
                    result.append(num.substr(decimalPos + 1, exponent));
                    result.append(".");
                    result.append(num.substr(decimalPos + exponent + 1, ePos - (decimalPos + exponent + 1)));
                }
            }
        } else {  // decimal point moving left
            if (decimalPos == string::npos) {
                if (((int)ePos + exponent) > 0) {  // 123e-2
                    result.reserve(ePos + exponent + 2 + std::abs(exponent));
                    result = num.substr(0, ePos + exponent);
                    result.append(".");
                    result.append(num.substr(ePos + exponent, std::abs(exponent)));
                } else {  // 123e-4
                    result.reserve(3 + std::abs(exponent));
                    result.append("0.");
                    result.append(std::abs(exponent) - ePos, '0');
                    result.append(num.substr(0, ePos));
                }
            } else {
                if (static_cast<decltype(decimalPos)>(std::abs(exponent)) >= decimalPos) {  // 12.12e-2
                    result.reserve(std::abs(exponent) + ePos - decimalPos);
                    result.append("0.");
                    result.append(std::abs(exponent) - decimalPos, '0');
                    result.append(num.substr(0, decimalPos));
                    result.append(num.substr(decimalPos + 1, ePos - decimalPos - 1));
                } else {  // 12.12e-1
                    result.reserve(ePos + 1 + std::abs(exponent));
                    result = num.substr(0, decimalPos - std::abs(exponent));
                    result.append(".");
                    result.append(num.substr(decimalPos + 1, ePos - decimalPos - 1));
                }
            }
        }
    } else {  // 123
        result = num;
    }
    return result;
}
}

#endif  // MIDAS_FLOAT_HELPER_H
