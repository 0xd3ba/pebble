#include <gtest/gtest.h>
#include "stats/running_avg.hpp"

using namespace pebble::stats;

namespace {
constexpr double kEpsilon = 1e-9;
}  // namespace

TEST(RunningAverageTest, NewAverageIsZero) {
    RunningAverage avg{"ipc"};
    EXPECT_EQ(avg.count(), 0u);
    EXPECT_DOUBLE_EQ(avg.mean(), 0.0);
    EXPECT_DOUBLE_EQ(avg.variance(), 0.0);
    EXPECT_EQ(avg.name(), "ipc");
}

TEST(RunningAverageTest, SingleSampleMeanEqualsThatSample) {
    RunningAverage avg{"x"};
    avg.add(5.0);
    EXPECT_EQ(avg.count(), 1u);
    EXPECT_DOUBLE_EQ(avg.mean(), 5.0);
    EXPECT_DOUBLE_EQ(avg.variance(), 0.0);
    EXPECT_DOUBLE_EQ(avg.stddev(), 0.0);
}

TEST(RunningAverageTest, MeanOfSimpleSequence) {
    RunningAverage avg{"x"};
    avg.add(2.0);
    avg.add(4.0);
    avg.add(6.0);
    EXPECT_NEAR(avg.mean(), 4.0, kEpsilon);
    EXPECT_EQ(avg.count(), 3u);
}

TEST(RunningAverageTest, VarianceMatchesKnownPopulationValue) {
    // Values: 2, 4, 4, 4, 5, 5, 7, 9 => mean 5, population variance 4
    RunningAverage avg{"x"};
    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        avg.add(v);
    }
    EXPECT_NEAR(avg.mean(), 5.0, kEpsilon);
    EXPECT_NEAR(avg.variance(), 4.0, kEpsilon);
    EXPECT_NEAR(avg.stddev(), 2.0, kEpsilon);
}

TEST(RunningAverageTest, ConstantSequenceHasZeroVariance) {
    RunningAverage avg{"x"};
    avg.add(7.0);
    avg.add(7.0);
    avg.add(7.0);
    EXPECT_NEAR(avg.variance(), 0.0, kEpsilon);
}

TEST(RunningAverageTest, StddevIsSqrtOfVariance) {
    RunningAverage avg{"x"};
    for (double v : {1.0, 2.0, 3.0, 4.0, 5.0}) avg.add(v);
    EXPECT_NEAR(avg.stddev(), std::sqrt(avg.variance()), kEpsilon);
}

TEST(RunningAverageTest, LargeSampleCountStaysNumericallyStable) {
    // large offset + many samples would visibly drift with naive accumulation. Welford's should stay accurate
    RunningAverage avg{"x"};
    constexpr double kOffset = 1e9;
    for (int i=0; i<100000; i++) {
        avg.add(kOffset + (i % 2 == 0 ? 1.0 : -1.0));
    }

    EXPECT_NEAR(avg.mean(), kOffset, 1e-3);
    EXPECT_NEAR(avg.variance(), 1.0, 1e-3);
}
