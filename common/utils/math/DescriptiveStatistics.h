#ifndef MIDAS_DESCRIPTIVE_STATISTICS_H
#define MIDAS_DESCRIPTIVE_STATISTICS_H

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>

namespace midas {

class DescriptiveStatistics {
private:
    const size_t INFINITE_WINDOW = 0;
    size_t windowSize{INFINITE_WINDOW};
    std::deque<double> dq;

public:
    DescriptiveStatistics() {}

    DescriptiveStatistics(size_t window) { set_window_size(window); }

    void add_value(double v) {
        if (windowSize != INFINITE_WINDOW) {
            size_t n = size();
            if (n == windowSize) {
                dq.pop_front();
                dq.push_back(v);
            } else if (n < windowSize) {
                dq.push_back(v);
            }
        } else {
            dq.push_back(v);
        }
    }

    void sort() { std::sort(dq.begin(), dq.end()); }

    /**
     * favor percentile operation, those need sorted data
     * performance very poor if data volume is huge, use add_value and sort at last approach
     */
    void add_value_and_sort(double v) {
        add_value(v);
        insert_sort();
    }

    void remove_most_recent_value() { dq.pop_back(); }

    double replace_most_recent_value(double v) {
        double old = *(dq.end() - 1);
        *(dq.end() - 1) = v;
        return old;
    }

    double get_mean() {
        if (size() == 0) return 0;
        double sum = get_sum();
        return sum / size();
    }

    double get_geometric_mean() {
        if (size() == 0) return 0;
        double sumOfLog = 0;
        for (double x : dq) {
            sumOfLog += std::log(x);
        }
        return std::exp(sumOfLog / size());
    }

    double get_variance() { return _get_variance(true); }

    double get_population_variance() { return _get_variance(false); }

    double get_standard_deviation() {
        if (size() <= 1)
            return 0;
        else
            return std::sqrt(get_variance());
    }

    double get_skewness() {
        size_t n = size();
        if (n <= 2) return 0;

        double m = get_mean();
        double accum = 0.0;
        double accum2 = 0.0;

        for (double x : dq) {
            double d = x - m;
            accum += d * d;
            accum2 += d;
        }

        double variance = (accum - accum2 * accum2 / n) / (n - 1);
        double accum3 = 0.0;

        for (double x : dq) {
            double d = x - m;
            accum3 += d * d * d;
        }

        accum3 /= variance * std::sqrt(variance);
        return n / ((n - 1.0) * (n - 2.0)) * accum3;
    }

    double get_max() {
        if (size() == 0)
            return 0;
        else
            return *std::max_element(dq.begin(), dq.end());
    }

    double get_min() {
        if (size() == 0)
            return 0;
        else
            return *std::min_element(dq.begin(), dq.end());
    }

    size_t size() const { return dq.size(); }

    double get_sum() { return std::accumulate(dq.begin(), dq.end(), 0.0); }

    double get_sum_square() {
        double sumSq = 0.0;

        for (double x : dq) {
            sumSq += x * x;
        }

        return sumSq;
    }

    void clear() { dq.clear(); }

    size_t get_window_size() { return windowSize; }

    void set_window_size(size_t _windowSize) {
        windowSize = _windowSize;
        if (windowSize != INFINITE_WINDOW && windowSize < dq.size()) {
            auto count = dq.size() - windowSize;
            while (count > 0) {
                dq.pop_front();
                --count;
            }
        }
    }

    double get_element(int index) { return dq[index]; }

    double get_percentile(double p) {
        int target = static_cast<int>(size() * p);
        return dq[target];
    }

private:
    double _get_variance(bool isBiasCorrected) {
        if (size() <= 1) return 0;
        double mean = get_mean();

        double accum = 0.0;
        double dev = 0.0;
        double accum2 = 0.0;

        for (double x : dq) {
            dev = x - mean;
            accum += dev * dev;
            accum2 += dev;
        }

        double len = size();

        if (isBiasCorrected) {
            return (accum - accum2 * accum2 / len) / (len - 1.0);
        } else {
            return (accum - accum2 * accum2 / len) / len;
        }
    }

    /**
     * only the last element is not in order
     */
    void insert_sort() {
        if (size() <= 1) return;

        auto itr = dq.end() - 1;
        double key = *itr;
        --itr;
        while (itr >= dq.begin() && (key < *itr)) {
            *(itr + 1) = *itr;
            --itr;
        }
        *(itr + 1) = key;
    }
};
}

#endif
