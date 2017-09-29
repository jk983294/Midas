#ifndef COMMON_MIDAS_EXCEPTION_H
#define COMMON_MIDAS_EXCEPTION_H

#include <exception>
#include <sstream>
#include <string>

namespace midas {

class MidasException : public std::exception {
public:
    std::string errorMsg;
    std::string fullMsg;
    std::string file;
    int line;

public:
    MidasException() throw() {}
    MidasException(const std::string& errorMsg_, const char* file_, int line_)
        : errorMsg(errorMsg_), file(file_), line(line_) {
        std::ostringstream oss;
        oss << errorMsg << " from " << file << ":" << line;
        fullMsg = oss.str();
    }
    MidasException(const MidasException& e) : errorMsg(e.errorMsg), fullMsg(e.fullMsg), file(e.file), line(e.line) {}
    ~MidasException() {}

    const char* what() const throw() { return fullMsg.c_str(); }
};
}

#define THROW_MIDAS_EXCEPTION(data)                                 \
    {                                                               \
        std::ostringstream oss;                                     \
        oss << data;                                                \
        throw midas::MidasException(oss.str(), __FILE__, __LINE__); \
    }

#endif
