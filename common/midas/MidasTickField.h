#ifndef MIDAS_MIDAS_TICK_FIELD_H
#define MIDAS_MIDAS_TICK_FIELD_H

#include <boost/range.hpp>
#include <boost/unordered_map.hpp>
#include <deque>
#include <map>
#include "utils/ConvertHelper.h"

using namespace std;

namespace midas {
template <typename T>
struct MidasTickHasher : public std::unary_function<T, size_t> {
    size_t operator()(const T& t) const { return boost::hash_value(t); }
};

template <>
struct MidasTickHasher<boost::iterator_range<const char*>>
    : public std::unary_function<boost::iterator_range<const char*>, size_t> {
    size_t operator()(const boost::iterator_range<const char*>& t) const {
        return boost::hash_range(t.begin(), t.end());
    }
};

template <typename T>
struct CopyValue {
    void operator()(T& dest, const char* src, size_t n) { dest = T(src, src + n); }
    void operator()(T& dest, const char* begin, const char* end) { dest = T(begin, end); }
};
template <>
struct CopyValue<string> {
    void operator()(string& dest, const char* src, size_t n) { dest.assign(src, n); }
    void operator()(string& dest, const char* begin, const char* end) { dest.assign(begin, end); }
};

template <typename T>
struct CompareValue {
    const char* str;
    size_t n;
    CompareValue(const char* c, size_t n_) : str(c), n(n_) {}
    bool operator()(const T& v) { return (v == T(str, str + n)); }
};
template <>
struct CompareValue<string> {
    const char* str;
    size_t n;
    CompareValue(const char* c, size_t n_) : str(c), n(n_) {}
    bool operator()(const string& v) {
        if (v.size() == n) return v.compare(str) == 0;
        return false;
    }
};

static const char DefaultTickString[] = ";";
static const char* DefaultTickStringEnd = DefaultTickString + strlen(DefaultTickString);

template <typename T = boost::iterator_range<const char*>>
class TickField {
public:
    T key;
    T value;

public:
    TickField() : key(DefaultTickString, DefaultTickStringEnd), value(DefaultTickString, DefaultTickStringEnd) {}
    TickField(const T& key_, const T& value_) : key(key_), value(value_) {}
    TickField(const char* key_, const char* value_ = DefaultTickString)
        : key(key_, key_ + strlen(key_)), value(value_, value_ + strlen(value_)) {}
    string key_string() const {
        if (key[0] != DefaultTickString[0])
            return string(&key[0], key.size());
        else
            return string();
    }

    string value_string() const {
        if (!value.empty() && value[0] != DefaultTickString[0])
            return string(&value[0], value.size());
        else
            return string();
    }

    const T& get_key() const { return key; }
    void set_key(const T& key_) { key = key_; }
    void set_key(const char* key_) { CopyValue<T>()(key, key_, strlen(key_)); }
    void set_key(const char* key_, size_t n) { CopyValue<T>()(key, key_, n); }
    template <size_t s>
    void set_key(const char (&key_)[s]) {
        CopyValue<T>()(key, key_, strlen(key_));
    }
    template <typename V>
    void set_key(const V& v) {
        key = T(v.begin(), v.end());
    }

    const T& get_value() const { return value; }
    void set_value(const T& value_) { value = value_; }
    void set_value(const char* value_) { CopyValue<T>()(value, value_, strlen(value_)); }
    void set_value(const char* value_, size_t n) { CopyValue<T>()(value, value_, n); }
    void set_value(const char* begin, const char* end) { CopyValue<T>()(value, begin, end); }
    template <size_t s>
    void set_value(const char (&value_)[s]) {
        CopyValue<T>()(value, value_, strlen(value_));
    }
    template <typename V>
    void set_value(const V& v) {
        value = T(v.begin(), v.end());
    }

    bool is_valid() const { return (key[0] != DefaultTickString[0] && has_value()); }

    bool has_value() const { return (value.empty() || value[0] != DefaultTickString[0]); }

    int size() const {
        if (!value.empty() && value[0] == DefaultTickString[0]) return -1;
        return value.size();
    }

    template <typename StreamType>
    StreamType& print(StreamType& out) const {
        if (value.empty() || value[0] != DefaultTickString[0]) {
            return out << key << ' ' << value << (!value.empty() ? " ," : ",");
        }
        return out;
    }

    operator string() const { return value_string(); }
    operator double() const { return std::atof(&value[0]); }
    operator float() const { return std::atof(&value[0]); }
    operator int() const { return std::atoi(&value[0]); }
    operator unsigned int() const { return std::atoi(&value[0]); }
    operator long() const { return std::atol(&value[0]); }
    operator unsigned long() const { return std::atol(&value[0]); }
    TickField& operator=(const char* v) {
        set_value(v);
        return *this;
    }
    TickField& operator=(const string& v) {
        set_value(v);
        return *this;
    }
    TickField& operator=(double v) {
        char tmp[64];
        int len = float_2_string_trim(v, 6, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    TickField& operator=(float v) {
        char tmp[64];
        int len = float_2_string_trim(v, 6, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    TickField& operator=(int v) {
        char tmp[64];
        int len = int2str(v, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    TickField& operator=(unsigned int v) {
        char tmp[64];
        int len = int2str(v, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    TickField& operator=(long v) {
        char tmp[64];
        int len = int2str(v, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    TickField& operator=(unsigned long v) {
        char tmp[64];
        int len = int2str(v, tmp, 64);
        set_value(tmp, len);
        return *this;
    }
    template <size_t s>
    bool operator==(const char (&str)[s]) const {
        if ((size_t)value.size() != s - 1) return false;
        return std::equal(value.begin(), value.end(), str);
    }
    bool operator==(const char* str) const {
        if ((size_t)value.size() != strlen(str)) return false;
        return std::equal(value.begin(), value.end(), str);
    }
    template <size_t s>
    bool operator!=(const char (&str)[s]) const {
        return !(operator==(str));
    }
    bool operator!=(const char* str) const { return !(operator==(str)); }
    template <typename U>
    bool operator==(const U& u) const {
        return (U) * this == u;
    }

    bool to_int64(int8_t scaleFactor, int64_t& v) const {
        int rc = 0;
        int64_t val = str2int64(value, scaleFactor, &rc);
        if (0 == rc) {
            v = val;
            return true;
        }
        return false;
    }

    int64_t to_int64(int8_t scaleFactor) const { return str2int64(value, scaleFactor); }

    bool to_uint64(int8_t scaleFactor, uint64_t& v) const {
        int rc = 0;
        uint64_t val = str2uint64(value, scaleFactor, &rc);
        if (0 == rc) {
            v = val;
            return true;
        }
        return false;
    }

    uint64_t to_uint64(int8_t scaleFactor) const { return str2uint64(value, scaleFactor); }
};
}

#endif
