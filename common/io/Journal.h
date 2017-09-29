#ifndef MIDAS_JOURNAL_H
#define MIDAS_JOURNAL_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utils/convert/TimeHelper.h>
#include <zlib.h>
#include <functional>
#include <iostream>
#include <string>

using namespace std;

namespace midas {

class Journal {
public:
    const timeval birth;
    const long long totalBytesAtBirth;
    long long totalBytes;
    const string link;
    bool cut{false};  // trigger by admin command
    string path;

public:
    Journal(const string& dir, const string& suffix, long long totalBytesNew, ostream& err)
        : birth(now_timeval()),
          totalBytesAtBirth(totalBytesNew),
          totalBytes(totalBytesNew),
          link(dir + "/current"),
          path(dir + "/" + date_time(birth.tv_sec) + suffix) {
        remove(link.c_str());
        if (symlink(path.c_str(), link.c_str()) < 0) {
            err << "Journal symlink error " << path << " " << link << strerror(errno) << endl;
            return;
        }
    }
    virtual ~Journal() { remove(link.c_str()); }
    bool scribble(char* p, int n, long long totalBytesNew, ostream& err) {
        if (bad()) return false;

        long cc = totalBytesNew - totalBytes;               // output bytes
        long k = (totalBytes % static_cast<long long>(n));  // offset
        long m = k + cc - n;                                // wrap around bytes

        if (m > 0 ? (put(&p[k], cc - m, err) ? put(p, m, err) : false) : put(&p[k], cc, err)) {
            totalBytes = totalBytesNew;
            return true;
        }

        err << "data lost as write failed\n";
        shutdown();
        if (remove(link.c_str())) err << "Journal remove failed\n";
        return false;
    }
    const long bytes() const { return totalBytes - totalBytesAtBirth; }

    static bool get_epoch(time_t& epoch, const string& text) {
        unsigned int year, month, day, hour, minute, second;
        char dot;
        int n = sscanf(text.c_str(), "%4d%02d%02d%c%02d%02d%02d", &year, &month, &day, &dot, &hour, &minute, &second);
        if (n == 7 && --month < 12) {
            int days = --day;
            static int x[12]{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
            days += (year - 1970) * 365 + x[month];
            days += (year - 1969) / 4 + (year % 4 == 0 && month > 1 ? 1 : 0);
            epoch = days * 86400 + (hour * 60 + minute) * 60 + second;
            if (date_time(epoch) == text) return true;
        }
        return false;
    }

    static string date_time(time_t ct) {
        struct tm tm;
        localtime_r(&ct, &tm);
        char buffer[24];
        std::snprintf(buffer, sizeof buffer, "%4d%02d%02d.%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                      tm.tm_hour, tm.tm_min, tm.tm_sec);
        return buffer;
    }

    long pending_bytes(long long totalBytesNew) const { return totalBytesNew - totalBytes; }
    long free_bytes(int bufLen, long long totalBytesNew) const { return bufLen - pending_bytes(totalBytesNew); };

    void straggle(long long totalBytesNew) { totalBytes = totalBytesNew; }

    static bool create_directory(const string& dir, ostream& err) {
        struct stat d;
        if (stat(dir.c_str(), &d) < 0) {
            if (errno != ENOENT) err << "stat() fails " << strerror(errno) << endl;

            if (mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
                err << "mkdir() fails " << strerror(errno) << endl;
                return false;
            } else {
                return true;
            }
        }

        if (S_ISDIR(d.st_mode)) return true;
        err << dir << " is not directory!" << endl;
        return false;
    }

    virtual bool bad() const = 0;

    void set_cut() { cut = true; }

private:
    virtual bool put(char* p, int n, ostream&) = 0;

    virtual void shutdown() = 0;
};

class FJournal : public Journal {
private:
    FILE* f;

public:
    FJournal(const string& dir, const string& suffix, long long totalBytesNew, ostream& err)
        : Journal(dir, suffix, totalBytesNew, err) {
        f = fopen(path.c_str(), "w");
        if (!f) err << "fopen() failed " << strerror(errno) << endl;
    }
    ~FJournal() {
        if (f) fclose(f);
    }
    bool bad() const { return !f; }

private:
    virtual bool put(char* p, int n, ostream& err) {
        if (fwrite(p, n, 1, f) == size_t(1)) return true;
        err << "fwrite() failed " << strerror(errno) << endl;
        return false;
    }

    virtual void shutdown() {
        if (f) fclose(f);
        f = 0;
    }
};

class GzJournal : public Journal {
private:
    gzFile f;

public:
    GzJournal(const string& dir, const string& suffix, long long totalBytesNew, ostream& err)
        : Journal(dir, suffix, totalBytesNew, err) {
        f = gzopen(path.c_str(), "wb");
        if (!f) {
            if (errno)
                err << "gzopen() failed " << strerror(errno) << endl;
            else
                err << "gzopen() failed no enough memory" << endl;
        }
    }
    ~GzJournal() {
        if (f) gzclose(f);
    }
    bool bad() const { return !f; }

private:
    virtual bool put(char* p, int n, ostream& err) {
        if (gzwrite(f, p, n) == n) return true;
        if (errno)
            err << "gzwrite() failed " << strerror(errno) << endl;
        else
            err << "gzwrite() failed" << endl;
        return false;
    }

    virtual void shutdown() {
        if (f) gzclose(f);
        f = 0;
    }
};
}

#endif
