#include <cstdint>
#include <gtest/gtest.h>
#include "stats/counter.hpp"

using namespace pebble::stats;

TEST(CounterTest, NewCounterIsZero) {
    Counter c{"x"};
    EXPECT_EQ(c.value(), 0u);
    EXPECT_EQ(c.name(), "x");
}

TEST(CounterTest, IncrementDefaultsToOne) {
    Counter c{"x"};
    c.increment();
    EXPECT_EQ(c.value(), 1u);
}

TEST(CounterTest, IncrementByAmount) {
    Counter c{"x"};
    c.increment(5);
    c.increment(3);
    EXPECT_EQ(c.value(), 8u);
}

TEST(CounterTest, ResetClearsValue) {
    Counter c{"x"};
    c.increment(10);
    c.reset();
    EXPECT_EQ(c.value(), 0u);
}
