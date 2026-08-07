#include <cstdint>
#include <optional>
#include <stdexcept>
#include <gtest/gtest.h>
#include "primitives/register.hpp"
#include "uarch/prf.hpp"

using namespace pebble::uarch;
namespace primitives = pebble::primitives;

TEST(PhysicalRegisterFileTest, ConstructsWithValidSize) {
    EXPECT_NO_THROW(PhysicalRegisterFile{64});
    EXPECT_NO_THROW(PhysicalRegisterFile{kMaxPhysRegisters});
}

TEST(PhysicalRegisterFileTest, RejectsZeroSize) {
    EXPECT_THROW(PhysicalRegisterFile{0}, std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, RejectsSizeAboveCeiling) {
    EXPECT_THROW(PhysicalRegisterFile{kMaxPhysRegisters + 1}, std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, WriteThenReadReturnsSameValue) {
    PhysicalRegisterFile prf{64};
    const PhysRegId id{5};

    prf.write(id, 0xd3bad3ba);

    EXPECT_EQ(prf.read(id), 0xd3bad3ba);
}

TEST(PhysicalRegisterFileTest, DistinctIndicesAreIndependent) {
    PhysicalRegisterFile prf{64};
    const PhysRegId a{1};
    const PhysRegId b{2};

    prf.write(a, 111);
    prf.write(b, 222);

    EXPECT_EQ(prf.read(a), 111);
    EXPECT_EQ(prf.read(b), 222);
}

TEST(PhysicalRegisterFileTest, NewlyConstructedRegisterIsNotReady) {
    PhysicalRegisterFile prf{64};
    const PhysRegId id{3};

    EXPECT_FALSE(prf.is_ready(id));
}

TEST(PhysicalRegisterFileTest, WriteMakesRegisterReady) {
    PhysicalRegisterFile prf{64};
    const PhysRegId id{3};

    prf.write(id, 42);

    EXPECT_TRUE(prf.is_ready(id));
}

TEST(PhysicalRegisterFileTest, InvalidateClearsReadiness) {
    PhysicalRegisterFile prf{64};
    const PhysRegId id{3};

    prf.write(id, 42);
    prf.invalidate(id);

    EXPECT_FALSE(prf.is_ready(id));
    EXPECT_THROW(prf.read(id), pebble::primitives::InvalidRegisterRead);
}

TEST(PhysicalRegisterFileTest, ReadAtOrAboveConfiguredSizeThrows) {
    PhysicalRegisterFile prf{64};
    EXPECT_THROW(prf.read(PhysRegId{64}), std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, WriteAtOrAboveConfiguredSizeThrows) {
    PhysicalRegisterFile prf{64};
    EXPECT_THROW(prf.write(PhysRegId{64}, 0), std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, IsReadyAtOrAboveConfiguredSizeThrows) {
    PhysicalRegisterFile prf{64};
    EXPECT_THROW(prf.is_ready(PhysRegId{64}), std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, InvalidateAtOrAboveConfiguredSizeThrows) {
    PhysicalRegisterFile prf{64};
    EXPECT_THROW(prf.invalidate(PhysRegId{64}), std::invalid_argument);
}

TEST(PhysicalRegisterFileTest, IndexBelowCeilingButAboveConfiguredSizeIsRejected) {
    // Legal for Index<kMaxPhysRegs>, but this PRF was only configured for 64.
    PhysicalRegisterFile prf{64};
    EXPECT_THROW(prf.read(PhysRegId{200}), std::invalid_argument);
}

TEST(PhysicalRegisterFreeListTest, ConstructsWithValidSize) {
    EXPECT_NO_THROW(PhysicalRegisterFreeList{64});
}

TEST(PhysicalRegisterFreeListTest, AllocateReturnsDistinctIds) {
    PhysicalRegisterFreeList free_list{64};

    auto a = free_list.allocate();
    auto b = free_list.allocate();

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_NE(*a, *b);
}

TEST(PhysicalRegisterFreeListTest, AllocatedIdIsMarkedAllocated) {
    PhysicalRegisterFreeList free_list{64};

    auto id = free_list.allocate();

    ASSERT_TRUE(id.has_value());
    EXPECT_TRUE(free_list.is_allocated(*id));
}

TEST(PhysicalRegisterFreeListTest, ExhaustingPoolReturnsNullopt) {
    PhysicalRegisterFreeList free_list{2};

    EXPECT_TRUE(free_list.allocate().has_value());
    EXPECT_TRUE(free_list.allocate().has_value());
    EXPECT_EQ(free_list.allocate(), std::nullopt);
}

TEST(PhysicalRegisterFreeListTest, FreeingAllowsReallocation) {
    PhysicalRegisterFreeList free_list{1};
    auto id = free_list.allocate();
    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(free_list.allocate(), std::nullopt);

    free_list.free(*id);

    EXPECT_EQ(free_list.allocate(), id);
}

TEST(PhysicalRegisterFreeListTest, FreedIdIsNoLongerAllocated) {
    PhysicalRegisterFreeList free_list{64};
    auto id = free_list.allocate();
    ASSERT_TRUE(id.has_value());

    free_list.free(*id);

    EXPECT_FALSE(free_list.is_allocated(*id));
}

TEST(PhysicalRegisterFreeListTest, DoubleFreeThrows) {
    PhysicalRegisterFreeList free_list{64};
    auto id = free_list.allocate();
    ASSERT_TRUE(id.has_value());
    free_list.free(*id);

    EXPECT_THROW(free_list.free(*id), primitives::BitsetPoolError);
}

TEST(PhysicalRegisterFreeListTest, CheckpointCapturesAllocationState) {
    PhysicalRegisterFreeList free_list{64};
    auto id = free_list.allocate();
    ASSERT_TRUE(id.has_value());

    auto snapshot = free_list.checkpoint();
    free_list.free(*id);

    free_list.restore(snapshot);

    EXPECT_TRUE(free_list.is_allocated(*id));
}

TEST(PhysicalRegisterFreeListTest, RestoreAfterFurtherAllocationsRevertsThem) {
    PhysicalRegisterFreeList free_list{2};
    auto snapshot = free_list.checkpoint();  // nothing allocated yet

    auto a = free_list.allocate();
    auto b = free_list.allocate();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());

    free_list.restore(snapshot);

    EXPECT_FALSE(free_list.is_allocated(*a));
    EXPECT_FALSE(free_list.is_allocated(*b));
}

TEST(PhysicalRegisterFreeListTest, CheckpointIsIndependentOfSubsequentMutation) {
    PhysicalRegisterFreeList free_list{64};
    auto snapshot = free_list.checkpoint();

    auto id = free_list.allocate();
    ASSERT_TRUE(id.has_value());

    EXPECT_FALSE(snapshot.pool.is_allocated(id->index()));
}
