#include <gtest/gtest.h>
#include "primitives/event_queue.hpp"

using namespace pebble::primitives;

TEST(EventQueueTest, NewQueueIsEmptyAtCycleZero) {
    EventQueue q{};
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.current_cycle(), 0u);
    EXPECT_EQ(q.pending_event_count(), 0u);
}

TEST(EventQueueTest, ScheduleWithZeroDelayThrows) {
    EventQueue q{};
    EXPECT_THROW(q.schedule(0, [] {}), std::invalid_argument);
}

TEST(EventQueueTest, ScheduleIncreasesPendingCount) {
    EventQueue q{};
    q.schedule(1, [] {});
    q.schedule(2, [] {});
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.pending_event_count(), 2u);
}

TEST(EventQueueTest, AdvanceCycleFiresEventAtExactDelay) {
    EventQueue q{};
    bool fired = false;
    q.schedule(3, [&] { fired = true; });

    q.advance_cycle();  // cycle 1
    EXPECT_FALSE(fired);

    q.advance_cycle();  // cycle 2
    EXPECT_FALSE(fired);

    q.advance_cycle();  // cycle 3
    EXPECT_TRUE(fired);
}

TEST(EventQueueTest, FiredEventIsRemovedFromPending) {
    EventQueue q{};
    q.schedule(1, [] {});
    ASSERT_EQ(q.pending_event_count(), 1u);

    q.advance_cycle();
    EXPECT_EQ(q.pending_event_count(), 0u);
    EXPECT_TRUE(q.empty());
}

TEST(EventQueueTest, CurrentCycleAdvancesByOneEachCall) {
    EventQueue q{};
    q.advance_cycle();
    q.advance_cycle();
    q.advance_cycle();

    EXPECT_EQ(q.current_cycle(), 3u);
}

TEST(EventQueueTest, MultipleEventsSameCycleFireInInsertionOrder) {
    EventQueue q{};
    std::vector<int> log{};
    q.schedule(2, [&] { log.push_back(1); });
    q.schedule(2, [&] { log.push_back(2); });
    q.schedule(2, [&] { log.push_back(3); });

    q.advance_cycle();
    q.advance_cycle();

    ASSERT_EQ(log.size(), 3u);
    EXPECT_EQ(log[0], 1);
    EXPECT_EQ(log[1], 2);
    EXPECT_EQ(log[2], 3);
}

TEST(EventQueueTest, EventsAtDifferentCyclesFireOnCorrectCycleOnly) {
    EventQueue q{};
    std::vector<int> log{};
    q.schedule(1, [&] { log.push_back(100); });
    q.schedule(3, [&] { log.push_back(300); });

    q.advance_cycle();  // cycle 1: fires 100
    EXPECT_EQ(log, std::vector<int>({100}));

    q.advance_cycle();  // cycle 2: nothing
    EXPECT_EQ(log, std::vector<int>({100}));

    q.advance_cycle();  // cycle 3: fires 300
    EXPECT_EQ(log, std::vector<int>({100, 300}));
}

TEST(EventQueueTest, ReentrantScheduleDuringCallbackIsSafe) {
    /* A callback firing this cycle schedules another event further out;
     * must not corrupt the queue or the currently-firing callback vector
     * (this is exactly why advance_cycle() copies callbacks out before invoking them) */
    EventQueue q{};
    std::vector<int> log{};
    q.schedule(1, [&] {
        log.push_back(1);
        q.schedule(1, [&] { log.push_back(2); });
    });

    q.advance_cycle();  // cycle 1: fires first callback, schedules second
    EXPECT_EQ(log, std::vector<int>({1}));

    q.advance_cycle();  // cycle 2: fires chained callback
    EXPECT_EQ(log, std::vector<int>({1, 2}));
}

TEST(EventQueueTest, SchedulingPastEventsRelativeToCurrentCycleIsCumulative) {
    /* delay_cycles is always relative to current_cycle() at call time, not absolute.
     * Advancing first then scheduling should land on the expected absolute cycle */
    EventQueue q{};
    q.advance_cycle();
    q.advance_cycle();  // current_cycle() == 2

    bool fired = false;
    q.schedule(2, [&] { fired = true; });  // should fire at cycle 4

    q.advance_cycle();  // cycle 3
    EXPECT_FALSE(fired);

    q.advance_cycle();  // cycle 4
    EXPECT_TRUE(fired);
}
