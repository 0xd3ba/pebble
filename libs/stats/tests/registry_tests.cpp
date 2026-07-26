#include <string>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "stats/registry.hpp"

using namespace pebble::stats;

TEST(StatsRegistryTest, NewRegistryIsEmpty) {
    StatsRegistry reg{};
    EXPECT_EQ(reg.stats_count(), 0u);
}

TEST(StatsRegistryTest, RegisterCounterReturnsUsableReference) {
    StatsRegistry reg{};
    Counter& c = reg.register_counter("insts");
    c.increment(10);
    EXPECT_EQ(c.value(), 10u);
    EXPECT_EQ(reg.stats_count(), 1u);
}

TEST(StatsRegistryTest, RegisterHistogramReturnsUsableReference) {
    StatsRegistry reg{};
    Histogram& h = reg.register_histogram("latency");
    h.add(4);
    EXPECT_EQ(h.total_samples(), 1u);
}

TEST(StatsRegistryTest, RegisterRunningAverageReturnsUsableReference) {
    StatsRegistry reg{};
    RunningAverage& avg = reg.register_running_average("ipc");
    avg.add(1.5);
    avg.add(2.5);
    EXPECT_NEAR(avg.mean(), 2.0, 1e-9);
}

TEST(StatsRegistryTest, ReferenceStaysValidAfterFurtherRegistrations) {
    /* guards the unique_ptr storage design: registering more
     * stats afterward must not invalidate a previously returned
     * reference (which an unordered_map<string, Counter> by value could risk on
     * a rehash or rebalance, depending on container */
    StatsRegistry reg{};
    Counter& first = reg.register_counter("a");
    first.increment(1);

    reg.register_counter("b");
    reg.register_histogram("c");
    reg.register_running_average("d");

    first.increment(1);
    EXPECT_EQ(first.value(), 2u);
}

TEST(StatsRegistryTest, DuplicateNameSameTypeThrows) {
    StatsRegistry reg{};
    reg.register_counter("x");
    EXPECT_THROW(reg.register_counter("x"), std::invalid_argument);
}

TEST(StatsRegistryTest, DuplicateNameAcrossDifferentTypesThrows) {
    StatsRegistry reg{};
    reg.register_counter("x");
    EXPECT_THROW(reg.register_histogram("x"), std::invalid_argument);
    EXPECT_THROW(reg.register_running_average("x"), std::invalid_argument);
}

TEST(StatsRegistryTest, JsonDumpContainsCounterFields) {
    StatsRegistry reg{};
    Counter& c = reg.register_counter("retired");
    c.increment(42);

    nlohmann::json j = reg.to_json();
    ASSERT_TRUE(j.contains("retired"));
    EXPECT_EQ(j["retired"]["type"], "counter");
    EXPECT_EQ(j["retired"]["value"], 42);
}

TEST(StatsRegistryTest, JsonDumpContainsHistogramFields) {
    StatsRegistry reg{};
    Histogram& h = reg.register_histogram("latency");
    h.add(4);
    h.add(5);

    nlohmann::json j = reg.to_json();
    ASSERT_TRUE(j.contains("latency"));
    EXPECT_EQ(j["latency"]["type"], "histogram");
    EXPECT_EQ(j["latency"]["total_samples"], 2);
    EXPECT_EQ(j["latency"]["sum"], 9);
    ASSERT_TRUE(j["latency"]["buckets"].is_array());
}

TEST(StatsRegistryTest, JsonDumpContainsRunningAverageFields) {
    StatsRegistry reg{};
    RunningAverage& avg = reg.register_running_average("ipc");
    avg.add(1.0);
    avg.add(3.0);

    nlohmann::json j = reg.to_json();
    ASSERT_TRUE(j.contains("ipc"));
    EXPECT_EQ(j["ipc"]["type"], "running_average");
    EXPECT_EQ(j["ipc"]["count"], 2);
    EXPECT_NEAR(j["ipc"]["mean"].get<double>(), 2.0, 1e-9);
}

TEST(StatsRegistryTest, DumpJsonStringParsesBackToSameStructure) {
    StatsRegistry reg{};
    reg.register_counter("a").increment(7);

    const std::string dumped = reg.dump_json();
    nlohmann::json parsed = nlohmann::json::parse(dumped);
    EXPECT_EQ(parsed["a"]["value"], 7);
}

TEST(StatsRegistryTest, MixedStatsAllAppearInSingleDump) {
    StatsRegistry reg{};
    reg.register_counter("c1").increment(1);
    reg.register_histogram("h1").add(2);
    reg.register_running_average("r1").add(3.0);

    nlohmann::json j = reg.to_json();
    EXPECT_EQ(j.size(), 3u);
    EXPECT_TRUE(j.contains("c1"));
    EXPECT_TRUE(j.contains("h1"));
    EXPECT_TRUE(j.contains("r1"));
}
