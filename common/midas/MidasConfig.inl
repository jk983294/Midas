#ifndef MIDAS_MIDASCONFIG_INL
#define MIDAS_MIDASCONFIG_INL

namespace midas {

template <typename T>
inline T Config::getDefaultImpl(const std::string& path, const T& defaultValue) {
    scoped_lock_type lock(mtx);
    string adjustedPath(adjust_path(path));
    read_env(adjustedPath);
    return pt.get<T>(adjustedPath, defaultValue);
}

template <typename T>
inline bool Config::putDefaultImpl(const std::string& path, const T& defaultValue) {
    scoped_lock_type lock(mtx);
    string adjustedPath(adjust_path(path));
    pt.put<T>(adjustedPath, defaultValue);
    string strVal(pt.get<string>(adjustedPath));
    write_env(path, strVal);
    return true;
}

inline bool Config::convert_env_path(const std::string& path, std::string& value, std::string& key) {
    if (path.empty()) return false;
    if (path.find('.') == string::npos) {
        value = path;
        key = env_prefix() + path;
        return true;
    } else if (is_env_path(path)) {
        string tmp = path.substr(strlen(env_prefix()));
        if (tmp.find('.') == string::npos) {
            value = tmp;
            key = path;
            return true;
        }
    }
    return false;
}

inline string Config::adjust_path(const std::string& originalPath) {
    string newPath(originalPath);
    string value;
    convert_env_path(originalPath, value, newPath);
    auto itr = newPath.rbegin();
    if (itr == newPath.rend() || *itr == '.') {
        THROW_MIDAS_EXCEPTION("config bad path!");
    }
    return newPath;
}

inline bool Config::is_env_path(const string& path) { return (path.find(env_prefix()) == 0); }

inline bool Config::read_env(const string& path) {
    string value, key;
    if (convert_env_path(path, value, key)) {
        const char* val = std::getenv(value.c_str());
        if (val) {
            pt.put(key, val);
            return true;
        }
    }
    return false;
}

inline bool Config::write_env(const string& path, const string& value) {
    if (is_env_path(path)) {
        string envValue = path.substr(strlen(env_prefix()));
        if (setenv(envValue.c_str(), value.c_str(), true) == 0) return true;
    }
    return false;
}

inline void Config::dump_ptree(std::ostream& os) { boost::property_tree::info_parser::write_info(os, pt); }

template <typename T>
inline T Config::get(const std::string& path, const T& defaultValue) {
    string strVal = midas::__get<string>(*this, path, "");
    if (strVal.length() > 3) {
        bool finished = false;
        size_t startPos = 0;
        bool doneSubstitution = false;
        bool hasSubstitutionValue = false;

        static const string defaultV("__default__");

        while (!finished) {
            finished = true;
            startPos = strVal.find("$(", startPos);
            if (startPos != string::npos) {
                size_t endPos = strVal.find(')', startPos);
                if (endPos != string::npos) {
                    size_t subLen = endPos - startPos + 1;
                    string insidePath = strVal.substr(startPos + 2, subLen - 3);
                    hasSubstitutionValue = true;
                    string insideValue = __get(*this, insidePath, defaultV);
                    if (insideValue != defaultV) {
                        strVal.replace(startPos, subLen, insideValue);
                        doneSubstitution = true;
                    }

                    startPos++;
                    finished = false;
                }
            }
        }

        if (doneSubstitution) {
            put<string>("__TmpKey__", strVal);
            return __get<T>(*this, "__TmpKey__", defaultValue);
        } else if (hasSubstitutionValue) {
            return defaultValue;  // no sub found
        }
    }
    return __get<T>(*this, path, defaultValue);
}

inline std::string Config::get(const std::string& path, const char* defaultValue) {
    std::string dv(defaultValue);
    return get<std::string>(path, dv);
}

template <typename T>
inline T Config::getenv(const std::string& envValue, const T& defaultValue) {
    std::string path(env_prefix());
    path += envValue;
    return get<T>(path, defaultValue);
}

template <typename T>
inline bool Config::putenv(const std::string& envValue, const T& defaultValue) {
    std::string path(env_prefix());
    path += envValue;
    return put<T>(path, defaultValue);
}

inline bool Config::exists(const std::string& path) const {
    std::string adjPath = adjust_path(path);
    boost::optional<const boost::property_tree::ptree&> opt = pt.get_child_optional(adjPath);
    return (opt) ? true : false;
}

template <typename T>
inline bool Config::put(const std::string& path, const T& defaultValue) {
    return __put<T>(*this, path, defaultValue);
}

inline void Config::read_config(const std::string& fileName, const string& initPath) {
    Config tmpConfig;
    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::file_source(fileName));

    boost::property_tree::info_parser::read_info(in, tmpConfig.pt);
    merge_config(tmpConfig.pt, initPath);
}

inline void Config::merge_config(Config& other, const std::string& initPath) { merge_config(other.pt, initPath); }

inline void Config::merge_config(boost::property_tree::ptree& other, const std::string& initPath) {
    if (!initPath.empty()) {
        string adjustedPath = adjust_path(initPath);
        boost::optional<boost::property_tree::ptree&> opt = other.get_child_optional(adjustedPath);
        if (opt) merge_config_child(adjustedPath, *opt);
    } else
        merge_config_child(initPath, other);
}

inline void Config::merge_config_child(const std::string& parent, boost::property_tree::ptree& tree) {
    if (!parent.empty()) {
        put<string>(parent, tree.get_value<string>());
    }

    for (auto itr = tree.begin(); itr != tree.end(); ++itr) {
        string p(parent);
        if (!p.empty()) {
            p += ".";
        }
        p += itr->first;
        merge_config_child(p, itr->second);
    }
}
}

#endif  // MIDAS_MIDASCONFIG1_H
