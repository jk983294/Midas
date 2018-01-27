#include "catch.hpp"
#include "utils/math/DescriptiveStatistics.h"
#include "utils/math/MathHelper.h"

using namespace midas;

TEST_CASE("math helper", "[math]") {
    REQUIRE(is_equal(0.42, 0.42));
    REQUIRE(is_equal(0.42, 0.4200000001));
    REQUIRE(!is_equal(0.42, 0.4200000001, 1e-10));
    REQUIRE(is_zero(0.00000001));

    REQUIRE(is_same_sign(1, 2));
    REQUIRE(is_same_sign(1.42, 2.42));
    REQUIRE(is_same_sign(0.0, 0.42));
    REQUIRE(is_same_sign(0.0, -0.42));

    REQUIRE(is_same_sign_strong(1, 2));
    REQUIRE(is_same_sign_strong(1.42, 2.42));
    REQUIRE(!is_same_sign_strong(0.0, 0.42));
    REQUIRE(!is_same_sign_strong(0.0, -0.42));

    REQUIRE(is_in_range(3, 2, 5));
    REQUIRE(is_in_range(3, 5, 2));
    REQUIRE(!is_in_range(0, 2, 5));
    REQUIRE(!is_in_range(0, 5, 2));

    REQUIRE(is_in_range(3, 4, 2, 5));
    REQUIRE(is_in_range(3, 4, 5, 2));
    REQUIRE(!is_in_range(0, 3, 2, 5));
    REQUIRE(!is_in_range(0, 3, 5, 2));
}

TEST_CASE("DescriptiveStatistics", "[DescriptiveStatistics]") {
    std::vector<double> data{1, 2, 3, 4, 5, 6, 7, 8, 9};

    DescriptiveStatistics ds;
    ds.set_window_size(5);
    for (double aData : data) {
        ds.add_value(aData);
    }

    REQUIRE(ds.get_max() == 9.0);
    REQUIRE(ds.get_min() == 5.0);
    REQUIRE(ds.get_sum() == 35.0);
    REQUIRE(ds.get_sum_square() == 255.0);
    REQUIRE(ds.get_mean() == 7.0);
    REQUIRE(ds.get_geometric_mean() == 6.853467509397646);
    REQUIRE(ds.get_variance() == 2.5);
    REQUIRE(ds.get_population_variance() == 2.0);
    REQUIRE(ds.get_standard_deviation() == 1.5811388300841898);
    REQUIRE(ds.get_skewness() == 0.0);

    ds.clear();
    ds.set_window_size(1);
    for (double aData : data) {
        ds.add_value(aData);
    }

    REQUIRE(ds.get_max() == 9.0);
    REQUIRE(ds.get_min() == 9.0);
    REQUIRE(ds.get_sum() == 9.0);
    REQUIRE(ds.get_sum_square() == 81.0);
    REQUIRE(ds.get_mean() == 9.0);
    REQUIRE(is_equal(ds.get_geometric_mean(), 9.0));
    REQUIRE(ds.get_variance() == 0.0);
    REQUIRE(ds.get_population_variance() == 0.0);
    REQUIRE(ds.get_standard_deviation() == 0.0);
    REQUIRE(ds.get_skewness() == 0.0);

    ds.clear();
    REQUIRE(ds.get_max() == 0.0);
    REQUIRE(ds.get_min() == 0.0);
    REQUIRE(ds.get_sum() == 0.0);
    REQUIRE(ds.get_sum_square() == 0.0);
    REQUIRE(ds.get_mean() == 0.0);
    REQUIRE(is_equal(ds.get_geometric_mean(), 0.0));
    REQUIRE(ds.get_variance() == 0.0);
    REQUIRE(ds.get_population_variance() == 0.0);
    REQUIRE(ds.get_standard_deviation() == 0.0);
    REQUIRE(ds.get_skewness() == 0.0);

    std::vector<double> data1{9, 8, 7, 6, 5, 4, 3, 2, 1, 1.2, 8.5, 9.0};
    ds.set_window_size(0);
    for (double aData : data1) {
        ds.add_value(aData);
    }
    ds.sort();
    REQUIRE(ds.get_percentile(60) == 7.0);
    REQUIRE(ds.get_percentile(90) == 9.0);
}
