#include <sstream>
#include <unordered_set>
#include <gtest/gtest.h>
#include "isa/reg_id.hpp"

using namespace pebble::isa;

TEST(RegIdTest, ValidIndexConstructsSuccessfully) {
    RegId r{0};
    EXPECT_EQ(r.index(), 0);
}

TEST(RegIdTest, MaxValidIndexConstructsSuccessfully) {
    RegId r{31};
    EXPECT_EQ(r.index(), 31);
}

TEST(RegIdTest, IndexJustAboveRangeThrows) {
    EXPECT_THROW(RegId{32}, std::out_of_range);
}

TEST(RegIdTest, LargeOutOfRangeIndexThrows) {
    EXPECT_THROW(RegId{255}, std::out_of_range);
}

TEST(RegIdTest, EqualityComparesIndex) {
    EXPECT_EQ(RegId{5}, RegId{5});
    EXPECT_NE(RegId{5}, RegId{6});
}

TEST(RegIdTest, X0IsNotSpecialCasedAtConstruction) {
    // RegId(0) must construct like any other valid index
    EXPECT_NO_THROW(RegId{0});
    EXPECT_EQ(RegId{0}.index(), 0);
}

TEST(RegIdTest, StreamOutputFormatsAsAbiStyleName) {
    std::ostringstream oss;
    oss << RegId{10};
    EXPECT_EQ(oss.str(), "10");
}

TEST(RegIdTest, UsableAsUnorderedSetKey) {
    std::unordered_set<RegId> regs;
    regs.insert(RegId{1});
    regs.insert(RegId{2});
    regs.insert(RegId{1});  // duplicate
    EXPECT_EQ(regs.size(), 2u);
    EXPECT_TRUE(regs.count(RegId(1)));
    EXPECT_FALSE(regs.count(RegId(3)));
}
