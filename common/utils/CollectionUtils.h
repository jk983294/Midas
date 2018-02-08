#ifndef MIDAS_COLLECTION_UTILS_H
#define MIDAS_COLLECTION_UTILS_H

namespace midas {

template <typename M, typename V>
void map_keys(const M& m, V& v) {
    for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
        v.push_back(it->second);
    }
}

template <typename M, typename V>
void map_values(const M& m, V& v) {
    for (typename M::const_iterator it = m.begin(); it != m.end(); ++it) {
        v.push_back(it->second);
    }
}

template <typename M, typename Key>
bool is_exists(const M& m, const Key& key) {
    return m.find(key) != m.end();
}
}

#endif
