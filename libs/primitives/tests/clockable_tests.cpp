#include <gtest/gtest.h>
#include "primitives/clockable.hpp"

using namespace pebble::primitives;

class CountingClockable: public Clockable {
public:
    void tick() override { tick_count++; }
    void reset() override { tick_count = 0; }

    uint64_t tick_count{0};
};

TEST(ClockableTest, TickIncrementsCount) {
    CountingClockable c{};
    c.tick();
    c.tick();
    EXPECT_EQ(c.tick_count, 2);
}

TEST(ClockableTest, ResetClearsState) {
    CountingClockable c{};
    c.tick();
    c.tick();
    c.reset();
    EXPECT_EQ(c.tick_count, 0);
}
