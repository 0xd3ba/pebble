#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include "primitives/bitset_pool.hpp"

using namespace pebble::primitives;

TEST(BitsetPoolTest, ZeroCapacityThrowsOnConstruction) {
    EXPECT_THROW(BitsetPool(0), std::invalid_argument);
}

TEST(BitsetPoolTest, NewPoolIsEmptyNotFull) {
    BitsetPool pool{8};
    EXPECT_TRUE(pool.empty());
    EXPECT_FALSE(pool.full());
    EXPECT_EQ(pool.allocated_count(), 0u);
    EXPECT_EQ(pool.free_count(), 8u);
    EXPECT_EQ(pool.capacity(), 8u);
}

TEST(BitsetPoolTest, FirstAllocateReturnsIndexZero) {
    BitsetPool pool{8};
    auto idx = pool.allocate();
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(*idx, 0u);
}

TEST(BitsetPoolTest, AllocateReturnsLowestFreeIndexInOrder) {
    BitsetPool pool{4};
    EXPECT_EQ(pool.allocate(), 0u);
    EXPECT_EQ(pool.allocate(), 1u);
    EXPECT_EQ(pool.allocate(), 2u);
    EXPECT_EQ(pool.allocate(), 3u);
}

TEST(BitsetPoolTest, AllocateUpdatesAllocatedAndFreeCounts) {
    BitsetPool pool{4};
    pool.allocate();
    pool.allocate();
    EXPECT_EQ(pool.allocated_count(), 2u);
    EXPECT_EQ(pool.free_count(), 2u);
}

TEST(BitsetPoolTest, AllocateAllMakesPoolFull) {
    BitsetPool pool{4};
    for (int i=0; i<4; i++) pool.allocate();
    EXPECT_TRUE(pool.full());
    EXPECT_EQ(pool.allocated_count(), 4u);
}

TEST(BitsetPoolTest, AllocateOnExhaustedPoolReturnsNullopt) {
    BitsetPool pool{2};
    pool.allocate();
    pool.allocate();
    EXPECT_FALSE(pool.allocate().has_value());
    // Exhaustion must not corrupt state -- pool still reports full/counts correctly and a subsequent free() + allocate() still works
    EXPECT_TRUE(pool.full());
}

TEST(BitsetPoolTest, FreeThenAllocateReusesIndex) {
    BitsetPool pool{4};
    pool.allocate();          // 0
    auto b = pool.allocate(); // 1
    pool.allocate();          // 2
    pool.free(*b);
    auto reused = pool.allocate();
    ASSERT_TRUE(reused.has_value());
    EXPECT_EQ(*reused, *b);
}

TEST(BitsetPoolTest, FreeDecreasesAllocatedCount) {
    BitsetPool pool{4};
    pool.allocate();
    auto b = pool.allocate();
    pool.free(*b);
    EXPECT_EQ(pool.allocated_count(), 1u);
    EXPECT_EQ(pool.free_count(), 3u);
}

TEST(BitsetPoolTest, FreeOnNeverAllocatedIndexThrows) {
    BitsetPool pool{4};
    EXPECT_THROW(pool.free(2), BitsetPoolError);
}

TEST(BitsetPoolTest, DoubleFreeThrows) {
    BitsetPool pool{4};
    auto idx = pool.allocate();
    pool.free(*idx);
    EXPECT_THROW(pool.free(*idx), BitsetPoolError);
}

TEST(BitsetPoolTest, FreeOutOfRangeIndexThrows) {
    BitsetPool pool{4};
    EXPECT_THROW(pool.free(4), BitsetPoolError);
    EXPECT_THROW(pool.free(1000), BitsetPoolError);
}

TEST(BitsetPoolTest, IsAllocatedOutOfRangeThrows) {
    BitsetPool pool{4};
    EXPECT_THROW(pool.is_allocated(4), std::out_of_range);
}

TEST(BitsetPoolTest, IsAllocatedReflectsState) {
    BitsetPool pool{4};
    auto idx = pool.allocate();
    EXPECT_TRUE(pool.is_allocated(*idx));
    EXPECT_FALSE(pool.is_allocated((*idx + 1) % 4));
    pool.free(*idx);
    EXPECT_FALSE(pool.is_allocated(*idx));
}

/* Word-boundary correctness (capacity spans multiple 64-bit words) */
TEST(BitsetPoolTest, AllocatesAcrossWordBoundary) {
    // capacity 65 forces two backing words; index 64 lives in word 1
    BitsetPool pool{65};
    for (std::size_t i=0; i<64; i++) {
        auto idx = pool.allocate();
        ASSERT_TRUE(idx.has_value());
        EXPECT_EQ(*idx, i);
    }
    auto last = pool.allocate();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(*last, 64u);
    EXPECT_TRUE(pool.full());
}

TEST(BitsetPoolTest, FreeingFirstWordIndexWhileSecondWordFullStillWorks) {
    BitsetPool pool{65};
    for (int i=0; i<65; i++) pool.allocate();
    ASSERT_TRUE(pool.full());
    pool.free(0);
    auto idx = pool.allocate();
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(*idx, 0u);  // lowest free, even though it's in word and word 1 (containing index 64) is still full
}

/* --- Padding-bit safety (capacity not a multiple of 64) */
TEST(BitsetPoolTest, PaddingBitsAreNeverAllocatable) {
    /* capacity 70 => backing has 2 words = 128 bits, 58 of which are padding beyond the real range.
     * None of those must ever be handed out by allocate() */
    BitsetPool pool{70};
    std::vector<std::size_t> allocated;
    for (int i=0; i<70; i++) {
        auto idx = pool.allocate();
        ASSERT_TRUE(idx.has_value());
        allocated.push_back(*idx);
    }
    EXPECT_TRUE(pool.full());
    EXPECT_FALSE(pool.allocate().has_value());  // not 58 more indices available

    for (auto idx : allocated) {
        EXPECT_LT(idx, 70u);
    }
}

TEST(BitsetPoolTest, NonMultipleOf64CapacityReportsCorrectly) {
    BitsetPool pool{70};
    EXPECT_EQ(pool.capacity(), 70u);
    EXPECT_EQ(pool.free_count(), 70u);
}
