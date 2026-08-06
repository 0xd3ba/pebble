#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "utils/tracer.hpp"

using namespace pebble::utils;

TEST(TracerTest, DefaultConstructedIsEmpty) {
    Tracer<int, 4> t{};
    EXPECT_EQ(t.total_writes(), 0);
    EXPECT_EQ(t.size(), 4);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });
    EXPECT_TRUE(seen.empty());
}

TEST(TracerTest, SizeReflectsTemplateParameterNotFillLevel) {
    Tracer<int, 8> t{};
    EXPECT_EQ(t.size(), 8);   // capacity, not element count
    t.push_back(1);
    EXPECT_EQ(t.size(), 8);   // unchanged after a push
}

TEST(TracerTest, PushBelowCapacityPreservesOrder) {
    Tracer<int, 4> t{};
    t.push_back(10);
    t.push_back(20);
    t.push_back(30);

    EXPECT_EQ(t.total_writes(), 3);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });

    ASSERT_EQ(seen.size(), 3);
    EXPECT_EQ(seen[0], 10);
    EXPECT_EQ(seen[1], 20);
    EXPECT_EQ(seen[2], 30);
}

TEST(TracerTest, PushExactlyToCapacityNoWrap) {
    Tracer<int, 4> t{};
    for(int v : {1, 2, 3, 4}) t.push_back(v);

    EXPECT_EQ(t.total_writes(), 4);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });

    ASSERT_EQ(seen.size(), 4);
    EXPECT_EQ(seen, (std::vector<int>{1, 2, 3, 4}));
}

TEST(TracerTest, PushOneBeyondCapacityDropsOldest) {
    Tracer<int, 4> t{};
    for (int v : {1, 2, 3, 4, 5}) t.push_back(v);  // 1 should be evicted

    EXPECT_EQ(t.total_writes(), 5);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });

    ASSERT_EQ(seen.size(), 4);
    EXPECT_EQ(seen, (std::vector<int>{2, 3, 4, 5}));
}

TEST(TracerTest, MultipleWrapsKeepOnlyLatestN) {
    Tracer<int, 4> t{};
    // push 1 to 10; only the last 4 (7,8,9,10) should remain.
    for (int v=1; v<=10; v++) t.push_back(v);

    EXPECT_EQ(t.total_writes(), 10);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });

    ASSERT_EQ(seen.size(), 4);
    EXPECT_EQ(seen, (std::vector<int>{7, 8, 9, 10}));
}

TEST(TracerTest, OrderingAlwaysOldestToNewestAcrossWraps) {
    Tracer<int, 3> t{};
    for (int v=1; v<=7; v++) {
        t.push_back(v);
        std::vector<int> seen;
        t.for_each([&](const int& x) { seen.push_back(x); });
        // must always be strictly increasing (oldest -> newest) regardless of how many wraps have happened.
        ASSERT_TRUE(std::is_sorted(seen.begin(), seen.end()));
    }
}

TEST(TracerTest, TotalWritesIsMonotonicAndUncapped) {
    Tracer<int, 2> t{};
    for (int i=0; i<1000; i++) t.push_back(i);
    EXPECT_EQ(t.total_writes(), 1000);
    EXPECT_EQ(t.size(), 2); // capacity unaffected
}

TEST(TracerTest, CapacityOneAlwaysHoldsOnlyLatest) {
    Tracer<int, 1> t{};
    t.push_back(1);
    t.push_back(2);
    t.push_back(3);

    EXPECT_EQ(t.total_writes(), 3);

    std::vector<int> seen;
    t.for_each([&](const int& x) { seen.push_back(x); });

    ASSERT_EQ(seen.size(), 1);
    EXPECT_EQ(seen[0], 3);
}

TEST(TracerTest, WorksWithNonTrivialType) {
    Tracer<std::string, 3> t{};
    t.push_back("alpha");
    t.push_back("beta");
    t.push_back("gamma");
    t.push_back("delta"); // evicts "alpha"

    std::vector<std::string> seen;
    t.for_each([&](const std::string& s) { seen.push_back(s); });

    ASSERT_EQ(seen.size(), 3);
    EXPECT_EQ(seen, (std::vector<std::string>{"beta", "gamma", "delta"}));
}

TEST(TracerTest, ForEachIsReadOnlyAndRepeatable) {
    Tracer<int, 4> t{};
    for (int v : {1, 2, 3}) t.push_back(v);

    std::vector<int> first, second;
    t.for_each([&](const int& x) { first.push_back(x); });
    t.for_each([&](const int& x) { second.push_back(x); });

    EXPECT_EQ(first, second);
    EXPECT_EQ(t.total_writes(), 3); // unaffected by iteration
}

TEST(TracerTest, ForEachInvocationCountMatchesElementCount) {
    Tracer<int, 5> t{};
    for (int v=1; v<=8; v++) t.push_back(v); // wraps, 5 elements live

    int invocations = 0;
    long sum = 0;
    t.for_each([&](const int& x) {
        invocations++;
        sum += x;
    });

    EXPECT_EQ(invocations, 5);
    EXPECT_EQ(sum, 4+5+6+7+8);
}
