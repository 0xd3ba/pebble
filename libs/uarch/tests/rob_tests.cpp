#include <optional>
#include <gtest/gtest.h>
#include "isa/addr.hpp"
#include "isa/reg_id.hpp"
#include "uarch/prf.hpp"
#include "uarch/rob.hpp"

using namespace pebble::uarch;

namespace {

static RobEntry make_entry(pebble::isa::addr_t pc = 0) {
    return RobEntry{
        .pc = pc,
        .dest_arch_reg_id = pebble::isa::RegId{1},
        .dest_phys_reg_id = PhysRegId{10},
        .old_phys_reg_id = PhysRegId{5},
    };
}

}  // namespace

TEST(ReorderBufferTest, ConstructsWithValidCapacity) {
    EXPECT_NO_THROW(ReorderBuffer{64});
}

TEST(ReorderBufferTest, RejectsZeroCapacity) {
    EXPECT_THROW(ReorderBuffer{0}, std::invalid_argument);
}

TEST(ReorderBufferTest, RejectsCapacityAboveCeiling) {
    EXPECT_THROW(ReorderBuffer{kMaxRobEntries + 1}, std::invalid_argument);
}

TEST(ReorderBufferTest, StartsEmpty) {
    ReorderBuffer rob{4};

    EXPECT_TRUE(rob.empty());
    EXPECT_FALSE(rob.full());
    EXPECT_EQ(rob.size(), 0);
    EXPECT_EQ(rob.capacity(), 4);
}

TEST(ReorderBufferTest, AllocateReturnsIdAndIncreasesSize) {
    ReorderBuffer rob{4};

    auto id = rob.allocate(make_entry(0x1000));

    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(rob.size(), 1u);
}

TEST(ReorderBufferTest, AllocateFailsWhenFull) {
    ReorderBuffer rob{2};
    auto a = rob.allocate(make_entry());
    auto b = rob.allocate(make_entry());

    EXPECT_TRUE(rob.full());
    EXPECT_EQ(rob.allocate(make_entry()), std::nullopt);
}

TEST(ReorderBufferTest, AllocatedEntryIsReadableByIndexOperator) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry(0x2000));
    ASSERT_TRUE(id.has_value());

    EXPECT_EQ(rob[*id].pc, 0x2000);
    EXPECT_FALSE(rob[*id].completed);
}

TEST(ReorderBufferTest, MarkCompletedSetsCompletedFlag) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());
    ASSERT_TRUE(id.has_value());

    rob.mark_completed(*id);

    EXPECT_TRUE(rob[*id].completed);
}

TEST(ReorderBufferTest, IsHeadCompletedFalseOnEmpty) {
    ReorderBuffer rob{4};
    EXPECT_FALSE(rob.is_head_completed());
}

TEST(ReorderBufferTest, IsHeadCompletedFalseUntilMarked) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());
    ASSERT_TRUE(id.has_value());
    EXPECT_FALSE(rob.is_head_completed());

    rob.mark_completed(*id);

    EXPECT_TRUE(rob.is_head_completed());
}

TEST(ReorderBufferTest, RetireThrowsOnEmptyBuffer) {
    ReorderBuffer rob{4};
    EXPECT_THROW(rob.retire(), std::logic_error);
}

TEST(ReorderBufferTest, RetireThrowsWhenHeadNotCompleted) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());

    EXPECT_THROW(rob.retire(), std::logic_error);
}

TEST(ReorderBufferTest, RetireReturnsTheRetiredEntry) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry(0x3000));
    ASSERT_TRUE(id.has_value());
    rob.mark_completed(*id);

    RobEntry retired = rob.retire();

    EXPECT_EQ(retired.pc, 0x3000);
    EXPECT_EQ(retired.dest_phys_reg_id, PhysRegId{10});
    EXPECT_EQ(retired.old_phys_reg_id, PhysRegId{5});
}

TEST(ReorderBufferTest, RetireDecreasesSizeAndEmptiesBuffer) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());
    ASSERT_TRUE(id.has_value());
    rob.mark_completed(*id);

    RobEntry retired = rob.retire();

    EXPECT_TRUE(rob.empty());
    EXPECT_EQ(rob.size(), 0);
}

TEST(ReorderBufferTest, RetiresInProgramOrder) {
    ReorderBuffer rob{4};
    auto a = rob.allocate(make_entry(0x100));
    auto b = rob.allocate(make_entry(0x200));
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    rob.mark_completed(*b);  // completes out of order
    rob.mark_completed(*a);

    // but retirement must still happen oldest-first
    EXPECT_EQ(rob.retire().pc, 0x100);
    EXPECT_EQ(rob.retire().pc, 0x200);
}

TEST(ReorderBufferTest, AccessingRetiredSlotThrows) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());
    ASSERT_TRUE(id.has_value());
    rob.mark_completed(*id);
    RobEntry retired = rob.retire();

    EXPECT_THROW(rob[*id], std::invalid_argument);
}

TEST(ReorderBufferTest, MarkCompletedOnRetiredSlotThrows) {
    ReorderBuffer rob{4};
    auto id = rob.allocate(make_entry());
    ASSERT_TRUE(id.has_value());
    rob.mark_completed(*id);
    RobEntry retired = rob.retire();

    EXPECT_THROW(rob.mark_completed(*id), std::invalid_argument);
}

TEST(ReorderBufferTest, SlotIsReusableAfterRetire) {
    ReorderBuffer rob{1};
    auto a = rob.allocate(make_entry(0x100));
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->index(), 0);

    rob.mark_completed(*a);
    RobEntry retired = rob.retire();

    auto b = rob.allocate(make_entry(0x200));

    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(b->index(), 0);
    EXPECT_EQ(rob[*b].pc, 0x200);
}

TEST(ReorderBufferTest, EntryWithNoDestinationRegisterLeavesOptionalsEmpty) {
    ReorderBuffer rob{4};
    RobEntry store_entry{.pc = 0x400};  // no dest_arch_reg_id/dest_phys_reg_id

    auto id = rob.allocate(store_entry);
    ASSERT_TRUE(id.has_value());

    EXPECT_EQ(rob[*id].dest_arch_reg_id, std::nullopt);
    EXPECT_EQ(rob[*id].dest_phys_reg_id, std::nullopt);
}
