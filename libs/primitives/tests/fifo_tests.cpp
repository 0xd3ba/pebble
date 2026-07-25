#include <string>
#include <gtest/gtest.h>
#include "primitives/fifo.hpp"

using namespace pebble::primitives;

TEST(FifoQueueTest, ZeroCapacityThrowsOnConstruction) {
    EXPECT_THROW(FifoQueue<int>{0}, std::invalid_argument);
}

TEST(FifoQueueTest, NewQueueIsEmptyNotFull) {
    FifoQueue<int> q{4};
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());
    EXPECT_EQ(q.size(), 0u);
    EXPECT_EQ(q.capacity(), 4u);
}

TEST(FifoQueueTest, PushIncreasesOccupancy) {
    FifoQueue<int> q{4};
    q.push(1);
    EXPECT_EQ(q.size(), 1u);
    EXPECT_FALSE(q.empty());
    q.push(2);
    EXPECT_EQ(q.size(), 2u);
}

TEST(FifoQueueTest, PushToCapacityMakesFull) {
    FifoQueue<int> q{3};
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_TRUE(q.full());
    EXPECT_EQ(q.size(), 3u);
}

TEST(FifoQueueTest, PushOnFullThrows) {
    FifoQueue<int> q{2};
    q.push(1);
    q.push(2);
    EXPECT_THROW(q.push(3), FifoQueueFullError);
}

TEST(FifoQueueTest, PopOnEmptyThrows) {
    FifoQueue<int> q{2};
    EXPECT_THROW(q.pop(), FifoQueueEmptyError);
}

TEST(FifoQueueTest, FrontOnEmptyThrows) {
    FifoQueue<int> q{2};
    EXPECT_THROW(q.front(), FifoQueueEmptyError);
}

TEST(FifoQueueTest, PopReturnsInFifoOrder) {
    FifoQueue<int> q{3};
    q.push(10);
    q.push(20);
    q.push(30);
    EXPECT_EQ(q.pop(), 10);
    EXPECT_EQ(q.pop(), 20);
    EXPECT_EQ(q.pop(), 30);
}

TEST(FifoQueueTest, PopDecreasesOccupancyAndClearsFull) {
    FifoQueue<int> q{2};
    q.push(1);
    q.push(2);
    ASSERT_TRUE(q.full());
    q.pop();
    EXPECT_FALSE(q.full());
    EXPECT_EQ(q.size(), 1u);
}

TEST(FifoQueueTest, PopLastElementMakesEmpty) {
    FifoQueue<int> q{2};
    q.push(1);
    q.pop();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(FifoQueueTest, FrontDoesNotRemoveElement) {
    FifoQueue<int> q{2};
    q.push(42);
    EXPECT_EQ(q.front(), 42);
    EXPECT_EQ(q.size(), 1u);
    EXPECT_EQ(q.front(), 42);  // idempotent
    EXPECT_EQ(q.pop(), 42);
}

TEST(FifoQueueTest, WraparoundPreservesOrderAndCapacity) {
    // fill, fully drain, refill. exercises ring-buffer index wraparound
    FifoQueue<int> q{3};
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    // head has now wrapped past tail's initial position
    q.push(4);
    q.push(5);
    EXPECT_TRUE(q.full());
    EXPECT_EQ(q.pop(), 3);
    EXPECT_EQ(q.pop(), 4);
    EXPECT_EQ(q.pop(), 5);
    EXPECT_TRUE(q.empty());
}

TEST(FifoQueueTest, WorksWithNonTrivialType) {
    FifoQueue<std::string> q{2};
    q.push("alpha");
    q.push("beta");
    EXPECT_EQ(q.pop(), "alpha");
    EXPECT_EQ(q.pop(), "beta");
}

TEST(FifoQueueTest, CapacityIsStableAcrossUse) {
    FifoQueue<int> q{5};
    q.push(1);
    q.pop();
    EXPECT_EQ(q.capacity(), 5u);
}
