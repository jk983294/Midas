#ifndef MIDAS_LOG_WRITER_CONCRETE_H
#define MIDAS_LOG_WRITER_CONCRETE_H

#include <fstream>
#include <iomanip>
#include <iostream>
#include "LogFormat.h"
#include "LogOutput.h"
#include "LogWriter.h"

namespace midas {

class StdioLogWriter : public LogWriter {
public:
    typedef midas::LockType::mutex mutex;
    typedef midas::LockType::scoped_lock scoped_lock;

protected:
    mutex mutex_m;

public:
    virtual ~StdioLogWriter() {}
    void write_log(LogPriority priority, const char* file, int line, const std::string& msg) override final {
        scoped_lock lock(mutex_m);
        std::ostream& o = priority > WARNING ? std::cerr : std::cout;
        o << "[" << to_string(priority) << "]" << file << ":" << line << " " << msg << '\n';
    }
    virtual void flush_log() override final {}
};

class FileLogWriter : public LogWriter {
public:
    typedef midas::LockType::mutex mutex;
    typedef midas::LockType::scoped_lock scoped_lock;

    mutable mutex mtx;
    std::ofstream ofs;
    std::string file;  // log file name
    bool good;
    size_t capacity;  // if 0, then no capacity limit, otherwise when overflow then truncate
    size_t line;

public:
    FileLogWriter() : good(false), capacity(0), line(0) {}
    FileLogWriter(std::string& file_) : good(false), capacity(0), line(0) {
        if (!open(file_)) throw MidasException("failed to open file: " + file_, __FILE__, __LINE__);
    }
    virtual ~FileLogWriter() {
        scoped_lock lock(mtx);
        if (ofs.is_open()) ofs.close();
    }
    bool open(const std::string& f) {
        scoped_lock lock(mtx);
        if (!f.empty() && (file != f || !ofs.is_open())) {
            if (ofs.is_open()) ofs.close();
            line = 0;
            file = f;
            ofs.open(f.c_str(), std::ios::app | std::ios::binary);
            good = ofs.good();
        }
        return good;
    }
    bool is_open() {
        scoped_lock lock(mtx);
        return ofs.is_open();
    }
    void close() {
        try {
            scoped_lock lock(mtx);
            good = false;
            ofs.close();
        } catch (...) {
        }
    }
    void write_log(LogPriority priority, const char* file, int line, const std::string& msg) override final;
    virtual void flush_log() override final {}
};

typedef LogWriterT<midas::StdioOutput<>, midas::StdProcessFormat, ASYNC> StdProcessLogWriter;
}

#include "LogWriterConcrete.inl"

#endif  // MIDAS_LOGWRITERCONCRETE_H
