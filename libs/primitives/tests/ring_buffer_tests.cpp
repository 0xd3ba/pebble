#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "primitives/ring_buffer.hpp"

using namespace pebble::primitives;
using RejectBuffer = RingBuffer<int, 3, RingBufferPolicy::Reject>;
using OverwriteBuffer = RingBuffer<int, 3, RingBufferPolicy::Overwrite>;

TEST(RingBufferRejectTest, StartsEmpty) {
    RejectBuffer rb{};

    EXPECT_TRUE(rb.empty());
    EXPECT_FALSE(rb.full());
    EXPECT_EQ(rb.size(), 0);
}

TEST(RingBufferRejectTest, PushBackReturnsSequentialIndices) {
    RejectBuffer rb{};

    EXPECT_EQ(rb.push_back(10), 0);
    EXPECT_EQ(rb.push_back(20), 1);
    EXPECT_EQ(rb.push_back(30), 2);
}

TEST(RingBufferRejectTest, BecomesFullAtCapacity) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    EXPECT_TRUE(rb.full());
    EXPECT_EQ(rb.size(), 3);
}

TEST(RingBufferRejectTest, PushBackPastCapacityReturnsNullopt) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    EXPECT_EQ(rb.push_back(4), std::nullopt);
    EXPECT_EQ(rb.size(), 3);
}

TEST(RingBufferRejectTest, PopFrontFreesSlotForReuse) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    rb.pop_front();

    EXPECT_FALSE(rb.full());
    EXPECT_EQ(rb.size(), 2);
    EXPECT_TRUE(rb.push_back(4).has_value());
}

TEST(RingBufferRejectTest, FrontReturnsOldestEntry) {
    RejectBuffer rb{};
    rb.push_back(10);
    rb.push_back(20);

    EXPECT_EQ(rb.front(), 10);

    rb.pop_front();

    EXPECT_EQ(rb.front(), 20);
}

TEST(RingBufferRejectTest, PopFrontOnEmptyThrows) {
    RejectBuffer rb{};
    EXPECT_THROW(rb.pop_front(), std::logic_error);
}

TEST(RingBufferRejectTest, IndexOperatorAccessesCorrectSlot) {
    RejectBuffer rb{};
    auto idx = rb.push_back(42);

    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(rb[*idx], 42);
}

TEST(RingBufferRejectTest, WrapsAroundCorrectlyAfterPops) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.pop_front();  // removes 1, head advances
    rb.pop_front();  // removes 2, head advances

    auto idx = rb.push_back(4);  // tail wraps around to slot 0

    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(*idx, 0);
    EXPECT_EQ(rb.front(), 3);
    EXPECT_EQ(rb.size(), 2);
}

TEST(RingBufferRejectTest, ForEachVisitsOldestToNewestAfterWraparound) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.pop_front();
    rb.pop_front();
    rb.push_back(4);
    rb.push_back(5);  // buffer now holds {3, 4, 5}, wrapped

    std::vector<int> seen;
    rb.for_each([&](const int &v) { seen.push_back(v); });

    EXPECT_EQ(seen, (std::vector<int>{3, 4, 5}));
}

TEST(RingBufferRejectTest, TotalWritesCountsAllSuccessfulPushes) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.pop_front();
    rb.push_back(3);
    rb.push_back(4);
    EXPECT_EQ(rb.push_back(5), std::nullopt);  // rejected: must not count

    EXPECT_EQ(rb.total_writes(), 4);
}

TEST(RingBufferRejectTest, PopBackRemovesNewestEntry) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    rb.pop_back();

    EXPECT_EQ(rb.size(), 2);
    EXPECT_FALSE(rb.full());
    EXPECT_TRUE(rb.push_back(4).has_value());  // freed slot reusable
}

TEST(RingBufferRejectTest, PopBackOnEmptyThrows) {
    RejectBuffer rb{};
    EXPECT_THROW(rb.pop_back(), std::logic_error);
}

TEST(RingBufferRejectTest, PopBackWrapsCorrectlyAtZero) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.pop_front();  // head advances, tail still at 0 (wrapped)
    rb.push_back(4);  // tail wraps around to slot 0

    rb.pop_back(); // should remove 4, land back_index() on slot 2 (value 3)

    EXPECT_EQ(rb.back_index(), 2);
    EXPECT_EQ(rb.size(), 2);
}

TEST(RingBufferRejectTest, BackIndexMatchesLastPushedSlot) {
    RejectBuffer rb{};
    auto idx = rb.push_back(42);

    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(rb.back_index(), *idx);
}

TEST(RingBufferRejectTest, TruncateAfterKeepsUpToGivenIndex) {
    RejectBuffer rb{};
    rb.push_back(1);
    auto b = rb.push_back(2);
    rb.push_back(3);

    ASSERT_TRUE(b.has_value());

    rb.truncate_after(*b);  // should remove slot 2 (value 3)

    EXPECT_EQ(rb.size(), 2);
    EXPECT_EQ(rb.back_index(), *b);
}

TEST(RingBufferRejectTest, TruncateAfterWithNulloptEmptiesBuffer) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);

    rb.truncate_after(std::nullopt);

    EXPECT_TRUE(rb.empty());
}

TEST(RingBufferRejectTest, TruncateAfterInvalidIndexThrows) {
    RejectBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);

    EXPECT_THROW(rb.truncate_after(99), std::invalid_argument);
}

TEST(RingBufferRejectTest, TruncateAfterFreesSlotsForReallocation) {
    RejectBuffer rb{};
    auto a = rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    ASSERT_TRUE(a.has_value());

    rb.truncate_after(*a);

    EXPECT_EQ(rb.size(), 1);
    EXPECT_TRUE(rb.push_back(4).has_value());
    EXPECT_TRUE(rb.push_back(5).has_value());
}

TEST(RingBufferRejectTest, TruncateAfterOnAlreadyMatchingBackIsNoOp) {
    RejectBuffer rb{};
    rb.push_back(1);
    auto b = rb.push_back(2);

    ASSERT_TRUE(b.has_value());

    rb.truncate_after(*b);

    EXPECT_EQ(rb.size(), 2);
    EXPECT_EQ(rb.back_index(), *b);
}

TEST(RingBufferOverwriteTest, PushBackPastCapacityEvictsOldest) {
    OverwriteBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.push_back(4); // must overwrite 1, head advances to 2

    EXPECT_EQ(rb.size(), 3);
    EXPECT_EQ(rb.front(), 2);
}

TEST(RingBufferOverwriteTest, PushBackPastCapacityAlwaysSucceeds) {
    OverwriteBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);

    EXPECT_TRUE(rb.push_back(4).has_value());
}

TEST(RingBufferOverwriteTest, ForEachReflectsMostRecentNEntries) {
    OverwriteBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.push_back(4);
    rb.push_back(5);  // only {3, 4, 5} should remain

    std::vector<int> seen;
    rb.for_each([&](const int& v) { seen.push_back(v); });

    EXPECT_EQ(seen, (std::vector<int>{3, 4, 5}));
}

TEST(RingBufferOverwriteTest, TotalWritesCountsEvictingPushesToo) {
    OverwriteBuffer rb{};
    rb.push_back(1);
    rb.push_back(2);
    rb.push_back(3);
    rb.push_back(4);  // evicts 1, still counts as write

    EXPECT_EQ(rb.total_writes(), 4);
}