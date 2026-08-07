#include <optional>
#include <gtest/gtest.h>
#include "isa/reg_id.hpp"
#include "uarch/prf.hpp"
#include "uarch/rat.hpp"

using namespace pebble::uarch;
namespace isa = pebble::isa;

TEST(RegisterAliasTableTest, LookupOnUnmappedRegisterReturnsNullopt) {
    RegisterAliasTable rat{};
    EXPECT_EQ(rat.lookup(isa::RegId{1}), std::nullopt);
}

TEST(RegisterAliasTableTest, RemapOnUnmappedRegisterReturnsNullopt) {
    RegisterAliasTable rat{};
    EXPECT_EQ(rat.remap(isa::RegId{1}, PhysRegId{10}), std::nullopt);
}

TEST(RegisterAliasTableTest, LookupAfterRemapReturnsNewMapping) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});

    EXPECT_EQ(rat.lookup(isa::RegId{1}), PhysRegId{10});
}

TEST(RegisterAliasTableTest, SecondRemapReturnsPreviousMapping) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    auto old = rat.remap(isa::RegId{1}, PhysRegId{20});

    EXPECT_EQ(old, PhysRegId{10});
}

TEST(RegisterAliasTableTest, SecondRemapUpdatesLookup) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    rat.remap(isa::RegId{1}, PhysRegId{20});

    EXPECT_EQ(rat.lookup(isa::RegId{1}), PhysRegId{20});
}

TEST(RegisterAliasTableTest, DistinctArchRegistersAreIndependent) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    rat.remap(isa::RegId{2}, PhysRegId{20});

    EXPECT_EQ(rat.lookup(isa::RegId{1}), PhysRegId{10});
    EXPECT_EQ(rat.lookup(isa::RegId{2}), PhysRegId{20});
}

TEST(RegisterAliasTableTest, CheckpointCapturesCurrentMapping) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});

    auto snapshot = rat.checkpoint().snapshot;

    ASSERT_TRUE(snapshot.contains(isa::RegId{1}));
    EXPECT_EQ(snapshot.at(isa::RegId{1}), PhysRegId{10});
}

TEST(RegisterAliasTableTest, RestoreReplacesCurrentMapping) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    auto snapshot = rat.checkpoint().snapshot;

    rat.remap(isa::RegId{1}, PhysRegId{20});
    rat.remap(isa::RegId{2}, PhysRegId{30});
    rat.restore(RatSnapshot{.snapshot=snapshot});

    EXPECT_EQ(rat.lookup(isa::RegId{1}), PhysRegId{10});
    EXPECT_EQ(rat.lookup(isa::RegId{2}), std::nullopt);
}

TEST(RegisterAliasTableTest, CheckpointIsIndependentOfSubsequentMutation) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    auto snapshot = rat.checkpoint().snapshot;

    rat.remap(isa::RegId{1}, PhysRegId{20});

    EXPECT_EQ(snapshot.at(isa::RegId{1}), PhysRegId{10});
}

TEST(RegisterAliasTableTest, SameCheckpointComparisonWorks) {
    RegisterAliasTable rat{};
    rat.remap(isa::RegId{1}, PhysRegId{10});
    rat.remap(isa::RegId{2}, PhysRegId{20});

    auto snapshot_1 = rat.checkpoint();
    auto snapshot_2 = rat.checkpoint();

    EXPECT_EQ(snapshot_1, snapshot_2);

    rat.remap(isa::RegId{1}, PhysRegId{20});
    auto snapshot_3 = rat.checkpoint();

    EXPECT_NE(snapshot_2, snapshot_3);
    EXPECT_EQ(snapshot_3, snapshot_3);
}