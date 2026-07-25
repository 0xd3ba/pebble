#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "primitives/cam.hpp"

using namespace pebble::primitives;

TEST(CamTest, NewCamIsEmpty) {
    Cam<int, std::string> cam{};
    EXPECT_TRUE(cam.empty());
    EXPECT_EQ(cam.size(), 0u);
}

TEST(CamTest, InsertIncreasesSize) {
    Cam<int, std::string> cam{};
    cam.insert(1, "a");
    EXPECT_FALSE(cam.empty());
    EXPECT_EQ(cam.size(), 1u);
    cam.insert(2, "b");
    EXPECT_EQ(cam.size(), 2u);
}

TEST(CamTest, InsertReturnsDistinctHandles) {
    Cam<int, std::string> cam{};
    const CamHandle h1 = cam.insert(1, "a");
    const CamHandle h2 = cam.insert(1, "b");  // same key, distinct handle
    EXPECT_NE(h1, h2);
}

TEST(CamTest, LookupFirstMissReturnsNullopt) {
    Cam<int, std::string> cam{};
    cam.insert(1, "a");
    EXPECT_FALSE(cam.lookup_first(99).has_value());
}

TEST(CamTest, LookupFirstHitReturnsValue) {
    Cam<int, std::string> cam{};
    cam.insert(42, "answer");
    auto result = cam.lookup_first(42);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "answer");
}

TEST(CamTest, LookupMissReturnsEmptyVector) {
    Cam<int, std::string> cam{};
    cam.insert(1, "a");
    EXPECT_TRUE(cam.lookup(99).empty());
}

TEST(CamTest, LookupSingleMatchReturnsOneEntry) {
    Cam<int, std::string> cam{};
    const CamHandle h = cam.insert(7, "seven");
    auto matches = cam.lookup(7);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].handle, h);
    EXPECT_EQ(matches[0].value, "seven");
}

TEST(CamTest, LookupReturnsAllDuplicateKeyMatches) {
    // Models multiple in-flight stores to the same address.
    Cam<int, std::string> cam{};
    const CamHandle h1 = cam.insert(100, "store1");
    const CamHandle h2 = cam.insert(100, "store2");
    const CamHandle h3 = cam.insert(100, "store3");
    cam.insert(200, "unrelated");

    auto matches = cam.lookup(100);
    ASSERT_EQ(matches.size(), 3u);

    std::vector<CamHandle> handles;
    for (const auto& m : matches) handles.push_back(m.handle);
    EXPECT_NE(std::find(handles.begin(), handles.end(), h1), handles.end());
    EXPECT_NE(std::find(handles.begin(), handles.end(), h2), handles.end());
    EXPECT_NE(std::find(handles.begin(), handles.end(), h3), handles.end());
}

TEST(CamTest, RemoveExistingHandleReturnsTrueAndDecreasesSize) {
    Cam<int, std::string> cam{};
    const CamHandle h = cam.insert(1, "a");
    EXPECT_TRUE(cam.remove(h));
    EXPECT_EQ(cam.size(), 0u);
}

TEST(CamTest, RemoveUnknownHandleReturnsFalse) {
    Cam<int, std::string> cam{};
    cam.insert(1, "a");
    EXPECT_FALSE(cam.remove(9999));
    EXPECT_EQ(cam.size(), 1u);  // untouched
}

TEST(CamTest, DoubleRemoveReturnsFalseSecondTime) {
    Cam<int, std::string> cam{};
    const CamHandle h = cam.insert(1, "a");
    EXPECT_TRUE(cam.remove(h));
    EXPECT_FALSE(cam.remove(h));
}

TEST(CamTest, RemoveOneOfDuplicateKeyEntriesLeavesOthersSearchable) {
    // Models a store retiring/draining from the LSQ while sibling stores
    // to the same address remain live.
    Cam<int, std::string> cam{};
    const CamHandle h1 = cam.insert(100, "store1");
    const CamHandle h2 = cam.insert(100, "store2");

    ASSERT_TRUE(cam.remove(h1));

    auto matches = cam.lookup(100);
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0].handle, h2);
    EXPECT_EQ(matches[0].value, "store2");
}

TEST(CamTest, ContainsReflectsHandleLifetime) {
    Cam<int, std::string> cam{};
    const CamHandle h = cam.insert(1, "a");
    EXPECT_TRUE(cam.contains(h));
    cam.remove(h);
    EXPECT_FALSE(cam.contains(h));
}

TEST(CamTest, ContainsFalseForNeverInsertedHandle) {
    Cam<int, std::string> cam{};
    EXPECT_FALSE(cam.contains(12345));
}

TEST(CamTest, WorksWithNonTrivialKeyAndValue) {
    Cam<std::string, int> cam;
    cam.insert("addr_0x1000", 111);
    cam.insert("addr_0x1000", 222);
    auto matches = cam.lookup("addr_0x1000");
    EXPECT_EQ(matches.size(), 2u);
    EXPECT_TRUE(cam.lookup("addr_0x2000").empty());
}