#ifndef COMMON_UTILS_LOG_WRITER_CONCRETE_INL
#define COMMON_UTILS_LOG_WRITER_CONCRETE_INL

namespace midas {

inline void FileLogWriter::write_log(LogPriority priority_, const char* file_, int line_, const std::string& msg_) {
    scoped_lock lock(mtx);
    if (!good) return;
    if (capacity && ofs.tellp() >= (int)capacity) {
        ofs.close();
        ofs.open(file.c_str(), std::ios::trunc);
        good = ofs.good();
    }
    if (good) {
        ofs << "[" << to_string(priority_) << "] " << (++line) << " " << get_time_now() << " " << file_ << ":" << line_
            << " " << msg_ << '\n';
    }
}
}

#endif
