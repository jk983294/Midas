#ifndef MIDAS_MIDAS_TICK_H
#define MIDAS_MIDAS_TICK_H

#include "MidasTickField.h"

using namespace std;

namespace midas {

template <typename T = boost::iterator_range<const char*>>
class TMidasTick {
public:
    typedef TickField<T> Field;
    typedef boost::unordered_map<T, Field, MidasTickHasher<T>> TContainer;
    typedef std::deque<Field*> TIndexArray;

    TContainer fields;
    TIndexArray fieldIndex;

public:
    TMidasTick() {}
    ~TMidasTick() {}
    TMidasTick(const string& i) { parse(i.c_str(), i.size()); }
    TMidasTick(const char* msg, size_t size) { parse(msg, size); }
    TMidasTick(const TMidasTick<T>& rhs) { assign(rhs); }
    template <typename V>
    TMidasTick(const TMidasTick<V>& rhs) {
        assign(rhs);
    }

    TMidasTick& operator=(const TMidasTick<T>& rhs) {
        if (this != &rhs) assign(rhs);
        return *this;
    }
    template <typename V>
    TMidasTick& operator=(const TMidasTick<V>& rhs) {
        if (reinterpret_cast<std::uintptr_t>(this) != reinterpret_cast<std::uintptr_t>(&rhs)) assign(rhs);
        return *this;
    }

    // functor used to check end of parsing
    struct NoCompleteCheck {
        bool operator()(const Field& field) { return false; }
    };

    // functor called after parsing each field, when functor return true, then stop
    template <typename CheckComplete>
    size_t parse(const char* input, size_t inputSize, CheckComplete check) {
        const char *start = input, *end = input + inputSize;
        const char *comma, *space;
        clear();
        while (start < end && *start != DefaultTickString[0] &&
               (space = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ' '))) != end) {
            if ((comma = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ','))) == end) {
                break;
            }

            T fieldKey(start, space);
            Field newField(fieldKey, T(space + 1, (comma > space + 1 ? comma - 1 : comma)));
            fieldIndex.push_back(&(fields.insert(typename TContainer::value_type(fieldKey, newField)).first->second));

            if (check(newField)) break;

            start = comma;
            ++start;
        }
        return (start - input + 1);
    }

    size_t parse(const char* input, size_t inputSize) {
        return parse<NoCompleteCheck>(input, inputSize, NoCompleteCheck());
    }
    size_t parse(const char* input) { return parse<NoCompleteCheck>(input, strlen(input), NoCompleteCheck()); }

    // parse then merge with with current content
    size_t merge(const char* input, size_t inputSize) {
        const char *start = input, *end = input + inputSize;
        const char *comma, *space;
        clear();
        while (start < end && *start != DefaultTickString[0] &&
               (space = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ' '))) != end) {
            if ((comma = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ','))) == end) {
                break;
            }

            T fieldKey(start, space);
            auto iter = fields.find(fieldKey);
            if (iter == fields.end()) {
                Field newField(fieldKey, T(space + 1, (comma > space + 1 ? comma - 1 : comma)));
                fieldIndex.push_back(
                    &(fields.insert(typename TContainer::value_type(fieldKey, newField)).first->second));
            } else {
                iter->second.set_value(space + 1, (comma > space + 1 ? comma - 1 : comma));
            }

            start = comma;
            ++start;
        }
        return (start - input + 1);
    }

    /**
     * only extract fields from input msg that have been added to MidasTick object already before parsing
     * typical use case is when client knows what fields they are interested in.
     * client can use add function to register interested fields
     * @tparam CheckComplete
     * @param input
     * @param inputSize
     * @param check functor called after parsing each field, when functor return true, then stop
     * @return
     */
    template <typename CheckComplete>
    size_t parse_light(const char* input, size_t inputSize, CheckComplete check) {
        const char *start = input, *end = input + inputSize;
        const char *comma, *space;
        clear_light();
        size_t numInsertedFields = 0;
        while (start < end && *start != DefaultTickString[0] &&
               (space = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ' '))) != end) {
            if ((comma = std::find_if(start, end, std::bind2nd(std::equal_to<char>(), ','))) == end) {
                break;
            }

            T fieldKey(start, space);
            auto iter = fields.find(fieldKey);
            if (iter != fields.end()) {
                iter->second.set_value(T(space + 1, (comma > space + 1 ? comma - 1 : comma)));
                ++numInsertedFields;
                if (numInsertedFields == fields.size() || check(iter->second)) break;
            }

            start = comma;
            ++start;
        }
        return (start - input + 1);
    }

    size_t parse_light(const char* input, size_t inputSize) {
        return parse_light<NoCompleteCheck>(input, inputSize, NoCompleteCheck());
    }
    size_t parse_light(const char* input) {
        return parse_light<NoCompleteCheck>(input, strlen(input), NoCompleteCheck());
    }

    template <typename Stream>
    Stream& print(Stream& out) const {
        for (auto iter = fieldIndex.begin(), iterEnd = fieldIndex.end(); iter != iterEnd; ++iter) {
            (*iter)->second(out);
        }
        out << ";";
        out.flush();
        return out;
    }

    template <size_t s>
    const Field& get(const char (&key)[s]) const {
        auto iter = fields.find(T(key, key + s - 1));
        if (iter != fields.end()) return iter->second;
        return no_field();
    }

    const Field& get(const char* key) const {
        auto iter = fields.find(T(key, key + strlen(key)));
        if (iter != fields.end()) return iter->second;
        return no_field();
    }

    const Field& get(const string& key) const {
        auto iter = fields.find(T(key.c_str(), key.c_str() + key.size()));
        if (iter != fields.end()) return iter->second;
        return no_field();
    }

    template <size_t s>
    const Field& operator[](const char (&key)[s]) const {
        return get(key);
    }

    const Field& operator[](const char* key) const { return get(key); }

    const Field& operator[](const string& key) const { return get(key); }

    /**
     * add a field
     * @param key
     * @param addFront if true then field be added in front of all existing fields
     * @return
     */
    Field& add(const char* key, bool addFront = false) {
        auto iter = fields.find(T(key, key + strlen(key)));
        if (iter != fields.end()) return iter->second;

        // insert new field
        Field newField(T(key, key + strlen(key)), T(DefaultTickString, DefaultTickStringEnd));
        if (!addFront) {
            fieldIndex.push_back(
                &(fields.insert(typename TContainer::value_type(newField.get_key(), newField)).first->second));
            return *(fieldIndex.back());
        }
        fieldIndex.push_front(
            &(fields.insert(typename TContainer::value_type(newField.get_key(), newField)).first->second));
        return *(fieldIndex.front());
    }

    Field& add(const string& key, bool addFront = false) {
        auto iter = fields.find(T(key.c_str(), key.c_str() + key.size()));
        if (iter != fields.end()) return iter->second;

        // insert new field
        Field newField(T(key.c_str(), key.c_str() + key.size()), T(DefaultTickString, DefaultTickStringEnd));
        if (!addFront) {
            fieldIndex.push_back(
                &(fields.insert(typename TContainer::value_type(newField.get_key(), newField)).first->second));
            return *(fieldIndex.back());
        }
        fieldIndex.push_front(
            &(fields.insert(typename TContainer::value_type(newField.get_key(), newField)).first->second));
        return *(fieldIndex.front());
    }

    Field& set(const char* key, const char* value, size_t n) {
        Field& newField = add(key);
        newField.set_value(value, n);
        return newField;
    }

    template <typename V>
    Field& set(const char* key, const V& v) {
        Field& newField = add(key);
        newField = v;
        return newField;
    }

    size_t size() const { return fieldIndex.size(); }

    bool empty() const { return size() == 0; }

    const Field& at(size_t index) const {
        if (index >= fieldIndex.size()) return no_field();
        return *fieldIndex[index];
    }

    void clear() {
        fields.clear();
        fieldIndex.clear();
    }

    void clear_light() {
        for (size_t i = 0; i < size(); ++i) {
            fieldIndex[i]->set_value(DefaultTickString);
        }
    }

private:
    const Field& no_field() const {
        static Field noField;
        return noField;
    }

    template <typename V>
    void assign(const TMidasTick<V>& other) {
        clear();
        for (size_t i = 0; i < other.fieldIndex.size(); ++i) {
            const auto& field = *other.fieldIndex[i];
            Field newField(T(field.get_key().begin(), field.get_key().end()),
                           T(field.get_value().begin(), field.get_value().end()));
            fieldIndex.push_back(
                &(fields.insert(typename TContainer::value_type(newField.get_key(), newField)).first->second));
        }
    }
};

typedef TMidasTick<> MidasTick;
}

#endif
