#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "stats/histogram.hpp"

using namespace pebble::stats;

TEST(HistogramTest, NewHistogramIsEmpty) {
    Histogram h{"latency"};
    EXPECT_EQ(h.total_samples(), 0u);
    EXPECT_EQ(h.sum(), 0u);
    EXPECT_EQ(h.name(), "latency");
}

TEST(HistogramTest, BucketIndexZeroForValueZero) {
    EXPECT_EQ(Histogram::bucket_index(0), 0u);
}

TEST(HistogramTest, BucketIndexForPowersOfTwoBoundaries) {
    // bucket 1 = {1}, bucket 2 = {2,3}, bucket 3 = {4...7}, bucket 4 = {8...15}
    EXPECT_EQ(Histogram::bucket_index(1), 1u);
    EXPECT_EQ(Histogram::bucket_index(2), 2u);
    EXPECT_EQ(Histogram::bucket_index(3), 2u);
    EXPECT_EQ(Histogram::bucket_index(4), 3u);
    EXPECT_EQ(Histogram::bucket_index(7), 3u);
    EXPECT_EQ(Histogram::bucket_index(8), 4u);
    EXPECT_EQ(Histogram::bucket_index(15), 4u);
    EXPECT_EQ(Histogram::bucket_index(16), 5u);
}

TEST(HistogramTest, LargeValueGetsHighBucketNotClipped) {
    // no fixed upper bound - a huge value should get its own high bucket rather than being clamped into an "overflow" bucket
    const uint64_t big = uint64_t{1} << 40;
    EXPECT_EQ(Histogram::bucket_index(big), 41u);
}

TEST(HistogramTest, AddIncrementsCorrectBucket) {
    Histogram h{"latency"};
    h.add(5);  // bucket 3 ([4...7])
    EXPECT_EQ(h.bucket_count(3), 1u);
    EXPECT_EQ(h.bucket_count(2), 0u);
}

TEST(HistogramTest, AddAccumulatesTotalSamplesAndSum) {
    Histogram h{"latency"};
    h.add(1);
    h.add(2);
    h.add(4);
    EXPECT_EQ(h.total_samples(), 3u);
    EXPECT_EQ(h.sum(), 7u);
}

TEST(HistogramTest, MultipleValuesInSameBucketAccumulate) {
    Histogram h{"latency"};
    h.add(4);
    h.add(5);
    h.add(6);
    h.add(7);
    EXPECT_EQ(h.bucket_count(3), 4u);
}

TEST(HistogramTest, BucketsGrowDynamicallyForLargerValues) {
    Histogram h{"latency"};
    h.add(1);  // bucket 1
    EXPECT_LT(h.buckets().size(), 10u);
    h.add(uint64_t{1} << 20);  // forces growth to bucket 21
    EXPECT_GE(h.buckets().size(), 21u);
    EXPECT_EQ(h.bucket_count(21), 1u);
}

TEST(HistogramTest, BucketCountOutOfRangeReturnsZeroNotThrow) {
    Histogram h{"latency"};
    h.add(1);
    EXPECT_EQ(h.bucket_count(1000), 0u);  // doesn't throw, just 0
}

TEST(HistogramTest, ZeroValuesGoToBucketZero) {
    Histogram h{"latency"};
    h.add(0);
    h.add(0);
    EXPECT_EQ(h.bucket_count(0), 2u);
}
