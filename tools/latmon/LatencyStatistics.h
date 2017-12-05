#ifndef MIDAS_LATENCY_STATISTICS_H
#define MIDAS_LATENCY_STATISTICS_H

#include <iomanip>
#include <sstream>
#include "utils/math/DescriptiveStatistics.h"

using namespace std;

class LatencyStatistics {
public:
    midas::DescriptiveStatistics ds;
    size_t span{0};    // measure in second, 0 means infinite time
    double startTime;  // start time to record statistics

public:
    LatencyStatistics(size_t s = 0) : span(s) {}

    /**
     * add into statistics
     * @param rcvt receive time in latmon
     * @param time send time from exchange
     */
    void add(double rcvt, double time) {
        if (ds.size() == 0) {
            startTime = time;
        }

        ds.add_value(rcvt - time);

        if (span && startTime + span < time) {
            print_result();
            ds.clear();
            startTime = 0;
        }
    }

    void print_result() {
        if (ds.size()) {
            ostringstream os;
            os << setprecision(12) << "st " << startTime << " ,span " << span << " ,latmin " << ds.get_min()
               << " ,latmax " << ds.get_max() << " ,latmean " << ds.get_mean() << " ,stddev "
               << ds.get_standard_deviation();

            ds.sort();
            for (int i = 50; i <= 90; i += 10) {
                os << " ,p" << i << " " << ds.get_percentile(i * 0.01);
            }
            for (int i = 91; i <= 99; ++i) {
                os << " ,p" << i << " " << ds.get_percentile(i * 0.01);
            }
            for (int i = 1; i <= 9; ++i) {
                os << " ,p99." << i << " " << ds.get_percentile((990.0 + i) * 0.001);
            }
            cout << os.str() << ",;\n";
        }
    }
};

#endif
