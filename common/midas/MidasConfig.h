#ifndef MIDAS_MIDASCONFIG_H
#define MIDAS_MIDASCONFIG_H

#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/tokenizer.hpp>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <vector>
#include "Lock.h"
#include "MidasException.h"
#include "Singleton.h"
#include "utils/MidasBind.h"

using namespace std;

namespace midas {

/**
 * query key with default value, if key not found, default value will be returned
 * we don't implicitly put missing key's default value into config tree unless you do it explicitly
 * info config do not support include directive
 */
class Config {
public:
    typedef midas::LockType::mutex mutex_type;
    typedef midas::LockType::scoped_lock scoped_lock_type;
    typedef std::pair<std::string, std::string> child_pair;
    typedef std::vector<child_pair> child_v;
    typedef std::map<std::string, std::string> path_m;

    boost::property_tree::ptree pt;
    mutable mutex_type mtx;

public:
    Config() {}
    static Config& instance() { return Singleton<Config>::instance(); }

    static bool convert_env_path(const std::string& path, std::string& value, std::string& key);
    static string adjust_path(const std::string& originalPath);
    static const char* env_group() { return "env"; }
    static const char* env_prefix() { return "env."; }
    static bool is_env_path(const string& path);

    bool read_env(const string& path);  // read env to config
    bool write_env(const string& path, const string& value);

    void read_config(const std::string& fileName, const string& initPath = string());
    void write_config(const std::string& fileName) { boost::property_tree::info_parser::write_info(fileName, pt); }

    void dump_ptree(std::ostream& os);

    template <typename T>
    T get(const std::string& path, const T& defaultValue = T());

    // first try env data, then normal data
    std::string get(const std::string& path, const char* defaultValue);

    template <typename T>
    bool put(const std::string& path, const T& defaultValue);
    template <typename T>
    bool set(const std::string& path, const T& defaultValue) {
        return put(path, defaultValue);
    }

    template <typename T>
    T getenv(const std::string& envValue, const T& defaultValue = T());

    template <typename T>
    bool putenv(const std::string& envValue, const T& defaultValue = T());

    template <typename T>
    T getDefaultImpl(const std::string& path, const T& defaultValue);

    template <typename T>
    bool putDefaultImpl(const std::string& path, const T& defaultValue);

    void merge_config(Config& other, const std::string& initPath = string());

private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void merge_config(boost::property_tree::ptree& tree, const std::string& initPath);
    void merge_config_child(const std::string& parent, boost::property_tree::ptree& tree);

    bool exists(const std::string& path) const;

    friend class midas::Singleton<Config>;
};

template <typename T>
T __get(Config& config, const std::string& path, const T& defaultValue) {
    return config.getDefaultImpl(path, defaultValue);
}

template <typename T>
bool __put(Config& config, const std::string& path, const T& defaultValue) {
    return config.putDefaultImpl(path, defaultValue);
}
}

#include "MidasConfig.inl"

#endif  // MIDAS_MIDASCONFIG_H
