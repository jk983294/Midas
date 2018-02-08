#ifndef COMMON_UTILS_TIME_HELPER_H
#define COMMON_UTILS_TIME_HELPER_H

#include <sys/time.h>
#include <boost/date_time.hpp>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include "MidasNumberUtils.h"

using std::string;

namespace midas {

double timespec2double(const timespec& ts);

void double2timespec(double t, timespec& ts);

uint64_t timespec2nanos(const timespec& ts);

void nanos2timespec(uint64_t t, timespec& ts);

int timespec_cmp(const timespec& t1, const timespec& t2);

void timespec_diff(const timespec& t1, const timespec& t2, timespec diff);

int64_t nanoSinceEpoch();

uint64_t nanoSinceEpochU();

uint64_t ntime();

// YYYYMMDD format
uint32_t nano2date(uint64_t nano = 0);

std::string time_t2string(const time_t ct);
std::string ntime2string(uint64_t nano);
time_t time_t_from_ymdhms(int ymd, int hms);
uint64_t ntime_from_double(double ymdhms);

std::string timeval2string(const struct timeval& tv);

std::string timespec2string(const timespec& ts);

bool string_2_second_intraday(const string& str, const string& format, unsigned& secs);

bool string_2_microsecond_intraday(const string& str, const string& format, unsigned& msecs);

int second_2_string_intraday(unsigned secs, char* buffer);

bool second_2_string_intraday(unsigned secs, const string& format, string& buffer);

int microsecond_2_string_intraday(unsigned microsecond, char* buffer);

bool microsecond_2_string_intraday(unsigned microsecond, const string& format, string& buffer);

std::time_t ptime2time_t(const boost::posix_time::ptime& t);

timeval now_timeval();

std::string now_string();

/**
 * "23:59:59" to 235959
 */
int intraday_time_HMS(const char* str);
int intraday_time_HMS(const string& str);

/**
 * "23:59" to 2359
 */
int intraday_time_HM(const char* str);
int intraday_time_HM(const string& str);

/**
 * "20171112" to 20171112
 */
int cob(const char* str);
int cob(const string& str);

/**
 * "2017-11-12" to 20171112
 */
int cob_from_dash(const char* str);
int cob_from_dash(const string& str);

/**
 * "20171112 23:59:59" to time_t
 */
double time_string2double(const string& str);
}

#endif
