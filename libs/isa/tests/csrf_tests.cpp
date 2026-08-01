#include <gtest/gtest.h>
#include "isa/csrf.hpp"
#include "isa/trap.hpp"

using namespace pebble::isa;

TEST(CsrFileTest, NewCsrFileStartsAtZero) {
    CsrFile csr{};
    EXPECT_EQ(csr.read_cycle_low(), 0);
    EXPECT_EQ(csr.read_cycle_high(), 0);
    EXPECT_EQ(csr.read_instret_low(), 0);
    EXPECT_EQ(csr.read_instret_high(), 0);
    EXPECT_EQ(csr.read_mcause(), TrapKind::None);
}

TEST(CsrFileTest, IncrementCycleDefaultsToOne) {
    CsrFile csr{};
    csr.increment_cycle();
    EXPECT_EQ(csr.read_cycle_low(), 1);
}

TEST(CsrFileTest, IncrementCycleByAmount) {
    CsrFile csr{};
    csr.increment_cycle(100);
    csr.increment_cycle(50);
    EXPECT_EQ(csr.read_cycle_low(), 150);
}

TEST(CsrFileTest, IncrementInstretIndependentOfCycle) {
    CsrFile csr{};
    csr.increment_cycle(10);
    csr.increment_instret(3);
    EXPECT_EQ(csr.read_cycle_low(), 10);
    EXPECT_EQ(csr.read_instret_low(), 3);
}

TEST(CsrFileTest, CycleOverflowsLowIntoHigh) {
    CsrFile csr{};
    csr.increment_cycle(0xffffffff);  // fill low 32 bits exactly
    EXPECT_EQ(csr.read_cycle_low(), 0xffffffff);
    EXPECT_EQ(csr.read_cycle_high(), 0u);
    csr.increment_cycle(1);  // carries into high
    EXPECT_EQ(csr.read_cycle_low(), 0);
    EXPECT_EQ(csr.read_cycle_high(), 1);
}

TEST(CsrFileTest, InstretOverflowsLowIntoHigh) {
    CsrFile csr{};
    csr.increment_instret(0x100000000);
    EXPECT_EQ(csr.read_instret_low(), 0);
    EXPECT_EQ(csr.read_instret_high(), 1);
}

TEST(CsrFileTest, SetMcauseThenReadReturnsSameKind) {
    CsrFile csr{};
    csr.set_mcause(TrapKind::IllegalInstruction);
    EXPECT_EQ(csr.read_mcause(), TrapKind::IllegalInstruction);
}

TEST(CsrFileTest, SetMcauseOverwritesPreviousValue) {
    CsrFile csr{};
    csr.set_mcause(TrapKind::LoadAccessFault);
    csr.set_mcause(TrapKind::EnvironmentCallFromMMode);
    EXPECT_EQ(csr.read_mcause(), TrapKind::EnvironmentCallFromMMode);
}

TEST(CsrFileTest, ResetClearsAllState) {
    CsrFile csr{};
    csr.increment_cycle(1000);
    csr.increment_instret(500);
    csr.set_mcause(TrapKind::Breakpoint);

    csr.reset();

    EXPECT_EQ(csr.read_cycle_low(), 0);
    EXPECT_EQ(csr.read_cycle_high(), 0);
    EXPECT_EQ(csr.read_instret_low(), 0);
    EXPECT_EQ(csr.read_instret_high(), 0);
    EXPECT_EQ(csr.read_mcause(), TrapKind::None);
}
