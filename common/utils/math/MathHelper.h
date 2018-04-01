#ifndef MIDAS_MATH_HELPER_H
#define MIDAS_MATH_HELPER_H

#include <cmath>

namespace midas {

inline bool is_equal(double a, double b) { return std::abs(a - b) < 1e-6; }

inline bool is_equal(double a, double b, double epsilon) { return std::abs(a - b) < epsilon; }

inline bool is_zero(double v) { return is_equal(v, 0.0); }

inline bool is_same_sign(int x, int y) { return !(x > 0 && y < 0) && !(x < 0 && y > 0); }

inline bool is_same_sign(double x, double y) { return !(x > 0 && y < 0) && !(x < 0 && y > 0); }

/**
 * strong constrain, if x or y is zero, consider not same sign
 */
inline bool is_same_sign_strong(int x, int y) { return !(x == 0 || y == 0) && !(x > 0 && y < 0) && !(x < 0 && y > 0); }

inline bool is_same_sign_strong(double x, double y) {
    return !(x == 0 || y == 0) && !(x > 0 && y < 0) && !(x < 0 && y > 0);
}

/**
 * range
 */
inline bool is_in_range(double value, double low, double high) {
    return value > std::min(low, high) && value < std::max(low, high);
}

inline bool is_in_range(int value, int low, int high) {
    return value >= std::min(low, high) && value <= std::max(low, high);
}

/**
 * check if [x, y] is in range of [a,b]
 */
inline bool is_in_range(double x, double y, double a, double b) { return is_in_range(x, a, b) && is_in_range(y, a, b); }
}

#endif
