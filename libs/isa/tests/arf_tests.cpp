#include <gtest/gtest.h>
#include "isa/arf.hpp"

using namespace pebble::isa;

TEST(ArchRegisterFileTest, AllRegistersStartAtZero) {
    ArchRegisterFile rf{};
    for (std::uint8_t i=0; i<32; i++)
        EXPECT_EQ(rf.read(RegId(i)), 0u);
}

TEST(ArchRegisterFileTest, WriteThenReadRoundTrips) {
    ArchRegisterFile rf{};
    rf.write(RegId(5), 0xDEADBEEF);
    EXPECT_EQ(rf.read(RegId(5)), 0xDEADBEEFu);
}

TEST(ArchRegisterFileTest, WriteOverwritesPreviousValue) {
    ArchRegisterFile rf{};
    rf.write(RegId(1), 10);
    rf.write(RegId(1), 20);
    EXPECT_EQ(rf.read(RegId(1)), 20u);
}

TEST(ArchRegisterFileTest, WritesToDifferentRegistersAreIndependent) {
    ArchRegisterFile rf{};
    rf.write(RegId(1), 111);
    rf.write(RegId(2), 222);
    EXPECT_EQ(rf.read(RegId(1)), 111u);
    EXPECT_EQ(rf.read(RegId(2)), 222u);
}

TEST(ArchRegisterFileTest, X0AlwaysReadsZeroEvenAfterWrite) {
    ArchRegisterFile rf{};
    rf.write(RegId(0), 0xFFFFFFFF);
    EXPECT_EQ(rf.read(RegId(0)), 0u);
}

TEST(ArchRegisterFileTest, X0WriteDoesNotAffectOtherRegisters) {
    ArchRegisterFile rf{};
    rf.write(RegId(1), 42);
    rf.write(RegId(0), 999);
    EXPECT_EQ(rf.read(RegId(1)), 42u);
    EXPECT_EQ(rf.read(RegId(0)), 0u);
}

TEST(ArchRegisterFileTest, HighestRegisterX31WorksNormally) {
    ArchRegisterFile rf{};
    rf.write(RegId(31), 12345);
    EXPECT_EQ(rf.read(RegId(31)), 12345u);
}

TEST(ArchRegisterFileTest, ResetClearsAllRegistersIncludingNonX0) {
    ArchRegisterFile rf{};
    rf.write(RegId(1), 1);
    rf.write(RegId(31), 31);
    rf.reset();
    for (uint8_t i=0; i<32; i++)
        EXPECT_EQ(rf.read(RegId(i)), 0u);
}
