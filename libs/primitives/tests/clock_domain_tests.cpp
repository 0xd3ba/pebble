#include <gtest/gtest.h>
#include "primitives/clockable.hpp"
#include "primitives/clock_domain.hpp"

using namespace pebble::primitives;

namespace {

class SimpleClockable: public Clockable {
public:
    void tick() override { tick_count++; }
    void reset() override { tick_count = 0; }

    uint64_t tick_count{0};
};

class OrderSimpleClockable : public Clockable {
public:
    explicit OrderSimpleClockable(int id, std::vector<int>& log) : id_(id), log_(log) {}
    void tick() override { log_.push_back(id_); }
    void reset() override {}

private:
    int id_;
    std::vector<int>& log_;
};

}  // namespace

TEST(ClockDomainTest, ZeroDividerThrowsOnConstruction) {
    EXPECT_THROW(ClockDomain{0}, std::invalid_argument);
}

TEST(ClockDomainTest, RegisterNullThrows) {
    ClockDomain domain{1};
    EXPECT_THROW(domain.register_clockable(nullptr), std::invalid_argument);
}

TEST(ClockDomainTest, DividerOneTicksEveryGlobalCycle) {
    ClockDomain domain{1};
    SimpleClockable c{};
    domain.register_clockable(&c);

    for (std::uint64_t cycle=0; cycle<5; cycle++) {
        domain.maybe_tick(cycle);
    }
    
    EXPECT_EQ(c.tick_count, 5);
}

TEST(ClockDomainTest, DividerThreeTicksEveryThirdGlobalCycleStartingAtZero) {
    ClockDomain domain(3);
    SimpleClockable c{};
    domain.register_clockable(&c);

    // Global cycles 0 1 2 ... 8 => ticks expected on 0, 3, 6: 3 ticks total.
    for (std::uint64_t cycle=0; cycle<9; cycle++) {
        domain.maybe_tick(cycle);
    }
    EXPECT_EQ(c.tick_count, 3);
}

TEST(ClockDomainTest, MultipleMembersTickInRegistrationOrder) {
    ClockDomain domain(1);
    std::vector<int> log{};
    OrderSimpleClockable a{1, log};
    OrderSimpleClockable b{2, log};
    OrderSimpleClockable c{3, log};
    domain.register_clockable(&a);
    domain.register_clockable(&b);
    domain.register_clockable(&c);

    domain.maybe_tick(0);
    ASSERT_EQ(log.size(), 3u);
    EXPECT_EQ(log[0], 1);
    EXPECT_EQ(log[1], 2);
    EXPECT_EQ(log[2], 3);
}

TEST(ClockDomainTest, ResetAllResetsEveryMember) {
    ClockDomain domain(1);
    SimpleClockable a{};
    SimpleClockable b{};
    domain.register_clockable(&a);
    domain.register_clockable(&b);

    domain.maybe_tick(0);
    domain.maybe_tick(1);
    ASSERT_EQ(a.tick_count, 2);
    ASSERT_EQ(b.tick_count, 2);

    domain.reset_all();
    EXPECT_EQ(a.tick_count, 0);
    EXPECT_EQ(b.tick_count, 0);
}

TEST(ClockDomainTest, MemberCountAndDividerAccessors) {
    ClockDomain domain{4};
    SimpleClockable a;
    domain.register_clockable(&a);
    EXPECT_EQ(domain.divider(), 4u);
    EXPECT_EQ(domain.members_count(), 1u);
}

TEST(ClockDomainTest, TwoDomainsDecoupledFrequenciesMultiCycleDriver) {
    // Models the motivating scenario directly: core clock divider=1, DRAM clock divider=3, driven off one shared global cycle counter
    ClockDomain core{1};
    ClockDomain dram{3};
    SimpleClockable core_unit;
    SimpleClockable dram_unit;
    core.register_clockable(&core_unit);
    dram.register_clockable(&dram_unit);

    for (std::uint64_t cycle=0; cycle<10; cycle++) {
        core.maybe_tick(cycle);
        dram.maybe_tick(cycle);
    }

    EXPECT_EQ(core_unit.tick_count, 10);
    EXPECT_EQ(dram_unit.tick_count, 4);  // cycles 0,3,6,9
}
