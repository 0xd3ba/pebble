#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "primitives/register.hpp"
#include "uarch/prf.hpp"

using namespace pebble::uarch;

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
