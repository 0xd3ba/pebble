#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "isa/op.hpp"
#include "isa/ops.hpp"
#include "utils/cast.hpp"

using namespace pebble::isa;
using Cast = pebble::utils::Cast;

TEST(OpsTest, RegRegAdd) {
    EXPECT_EQ(compute_reg_reg(Op::ADD, 3, 4), 7);
}

TEST(OpsTest, RegRegSub) {
    EXPECT_EQ(compute_reg_reg(Op::SUB, 10, 3), 7);
}

TEST(OpsTest, RegRegSll) {
    EXPECT_EQ(compute_reg_reg(Op::SLL, 1, 4), 16);
}

TEST(OpsTest, RegRegSltSigned) {
    EXPECT_EQ(compute_reg_reg(Op::SLT, Cast::u32(-1), 1), 1);
}

TEST(OpsTest, RegRegSltuTreatsNegativeAsLarge) {
    EXPECT_EQ(compute_reg_reg(Op::SLTU, Cast::u32(-1), 1), 0);
}

TEST(OpsTest, RegRegXor) {
    EXPECT_EQ(compute_reg_reg(Op::XOR, 0b1010, 0b0110), 0b1100);
}

TEST(OpsTest, RegRegSrlLogicalZeroFills) {
    EXPECT_EQ(compute_reg_reg(Op::SRL, 0x80000000, 4), 0x08000000);
}

TEST(OpsTest, RegRegSraArithmeticSignFills) {
    EXPECT_EQ(Cast::i32(compute_reg_reg(Op::SRA, Cast::u32(-16), 2)), -4);
}

TEST(OpsTest, RegRegOr) {
    EXPECT_EQ(compute_reg_reg(Op::OR, 0b1010, 0b0101), 0b1111);
}

TEST(OpsTest, RegRegAnd) {
    EXPECT_EQ(compute_reg_reg(Op::AND, 0b1100, 0b1010), 0b1000);
}

TEST(OpsTest, RegRegMul) {
    EXPECT_EQ(compute_reg_reg(Op::MUL, 6, 7), 42);
}

TEST(OpsTest, RegRegMulhu) {
    // 0xffffffff * 0xffffffff = 0xfffffffe00000001; high 32 = 0xfffffffe
    EXPECT_EQ(compute_reg_reg(Op::MULHU, 0xffffffff, 0xffffffff), 0xfffffffe);
}

TEST(OpsTest, RegRegMulhSignedNegativeTimesNegative) {
    // (-1) * (-1) = 1, high 32 bits of the 64-bit signed result = 0
    EXPECT_EQ(compute_reg_reg(Op::MULH, Cast::u32(-1), Cast::u32(-1)), 0u);
}

TEST(OpsTest, RegRegMulhsuMixedSign) {
    // -1 (signed) * 1 (unsigned) = -1; high 32 bits = 0xffffffff
    EXPECT_EQ(compute_reg_reg(Op::MULHSU, Cast::u32(-1), 1), 0xffffffff);
}

TEST(OpsTest, RegRegDiv) {
    EXPECT_EQ(Cast::i32(compute_reg_reg(Op::DIV, 20, 3)), 6);
}

TEST(OpsTest, RegRegDivByZeroReturnsAllOnes) {
    EXPECT_EQ(compute_reg_reg(Op::DIV, 20, 0), 0xffffffff);
}

TEST(OpsTest, RegRegDivOverflowReturnsDividend) {
    EXPECT_EQ(Cast::i32(compute_reg_reg(Op::DIV, Cast::u32(INT32_MIN), Cast::u32(-1))), INT32_MIN);
}

TEST(OpsTest, RegRegDivuByZeroReturnsAllOnes) {
    EXPECT_EQ(compute_reg_reg(Op::DIVU, 20, 0), 0xffffffff);
}

TEST(OpsTest, RegRegRem) {
    EXPECT_EQ(Cast::i32(compute_reg_reg(Op::REM, 20, 3)), 2);
}

TEST(OpsTest, RegRegRemByZeroReturnsDividend) {
    EXPECT_EQ(compute_reg_reg(Op::REM, 20, 0), 20);
}

TEST(OpsTest, RegRegRemOverflowReturnsZero) {
    EXPECT_EQ(compute_reg_reg(Op::REM, Cast::u32(INT32_MIN), Cast::u32(-1)), 0u);
}

TEST(OpsTest, RegRegRemuByZeroReturnsDividend) {
    EXPECT_EQ(compute_reg_reg(Op::REMU, 20, 0), 20);
}

TEST(OpsTest, RegRegRejectsOutOfFamilyOp) {
    EXPECT_THROW(compute_reg_reg(Op::ADDI, 1, 2), std::invalid_argument);
    EXPECT_THROW(compute_reg_reg(Op::BEQ, 1, 2), std::invalid_argument);
}

TEST(OpsTest, RegImmAddi) {
    EXPECT_EQ(compute_reg_imm(Op::ADDI, 10, 5), 15);
}

TEST(OpsTest, RegImmAddiNegativeImmediate) {
    EXPECT_EQ(compute_reg_imm(Op::ADDI, 10, -3), 7);
}

TEST(OpsTest, RegImmSltiSigned) {
    EXPECT_EQ(compute_reg_imm(Op::SLTI, Cast::u32(-5), 0), 1);
}

TEST(OpsTest, RegImmSltiuUnsigned) {
    EXPECT_EQ(compute_reg_imm(Op::SLTIU, Cast::u32(-5), 1), 0);
}

TEST(OpsTest, RegImmXori) {
    EXPECT_EQ(compute_reg_imm(Op::XORI, 0b1010, 0b0110), 0b1100);
}

TEST(OpsTest, RegImmOri) {
    EXPECT_EQ(compute_reg_imm(Op::ORI, 0b1010, 0b0101), 0b1111);
}

TEST(OpsTest, RegImmAndi) {
    EXPECT_EQ(compute_reg_imm(Op::ANDI, 0b1100, 0b1010), 0b1000);
}

TEST(OpsTest, RegImmSlli) {
    EXPECT_EQ(compute_reg_imm(Op::SLLI, 1, 4), 16);
}

TEST(OpsTest, RegImmSrliZeroFills) {
    EXPECT_EQ(compute_reg_imm(Op::SRLI, 0x80000000, 4), 0x08000000);
}

TEST(OpsTest, RegImmSraiSignFills) {
    EXPECT_EQ(Cast::i32(compute_reg_imm(Op::SRAI, Cast::u32(-16), 2)), -4);
}

TEST(OpsTest, RegImmShamtMaskedTo5Bits) {
    // A defensively-out-of-range shamt (e.g. 33) should behave as if masked to 5 bits (33 & 0x1F == 1).
    EXPECT_EQ(compute_reg_imm(Op::SLLI, 1, 33), 2u);
}

TEST(OpsTest, RegImmRejectsOutOfFamilyOp) {
    EXPECT_THROW(compute_reg_imm(Op::ADD, 1, 2), std::invalid_argument);
    EXPECT_THROW(compute_reg_imm(Op::LUI, 1, 2), std::invalid_argument);
}

TEST(OpsTest, UpperLuiReturnsImmediateDirectly) {
    EXPECT_EQ(compute_upp_imm(Op::LUI, /*pc=*/0x1000, Cast::i32(0x12345000)), 0x12345000);
}

TEST(OpsTest, UpperLuiIgnoresPc) {
    // Lui result must not depend on pc at all.
    EXPECT_EQ(compute_upp_imm(Op::LUI, 0, 0x1000), compute_upp_imm(Op::LUI, 0xffFF, 0x1000));
}

TEST(OpsTest, UpperAuipcAddsPcAndImmediate) {
    EXPECT_EQ(compute_upp_imm(Op::AUIPC, 0x1000, 0x2000), 0x3000);
}

TEST(OpsTest, UpperRejectsOutOfFamilyOp) {
    EXPECT_THROW(compute_upp_imm(Op::ADD, 0, 0), std::invalid_argument);
}

TEST(OpsTest, BeqEqualIsTaken) {
    EXPECT_TRUE(compute_branch_taken(Op::BEQ, 5, 5));
}

TEST(OpsTest, BeqUnequalIsNotTaken) {
    EXPECT_FALSE(compute_branch_taken(Op::BEQ, 5, 6));
}

TEST(OpsTest, BneUnequalIsTaken) {
    EXPECT_TRUE(compute_branch_taken(Op::BNE, 5, 6));
}

TEST(OpsTest, BltSignedComparison) {
    EXPECT_TRUE(compute_branch_taken(Op::BLT, Cast::u32(-1), 0));
}

TEST(OpsTest, BgeSignedComparison) {
    EXPECT_TRUE(compute_branch_taken(Op::BGE, 5, 5));
    EXPECT_FALSE(compute_branch_taken(Op::BGE, Cast::u32(-1), 0));
}

TEST(OpsTest, BltuUnsignedTreatsNegativeAsLarge) {
    // -1 as unsigned is huge, so it's NOT < 0
    EXPECT_FALSE(compute_branch_taken(Op::BLTU, Cast::u32(-1), 0));
}

TEST(OpsTest, BgeuUnsignedComparison) {
    EXPECT_TRUE(compute_branch_taken(Op::BGEU, Cast::u32(-1), 0));
}

TEST(OpsTest, BranchRejectsOutOfFamilyOp) {
    EXPECT_THROW(compute_branch_taken(Op::ADD, 1, 2), std::invalid_argument);
    EXPECT_THROW(compute_branch_taken(Op::JAL, 1, 2), std::invalid_argument);
}

TEST(OpsTest, LbSignExtendsNegativeByte) {
    // raw byte 0xff (== -1 as int8_t) sign-extends to 0xffffffff
    EXPECT_EQ(format_load_value(Op::LB, 0xff), 0xffffffff);
}

TEST(OpsTest, LbSignExtendsPositiveByte) {
    EXPECT_EQ(format_load_value(Op::LB, 0x7f), 0x0000007f);
}

TEST(OpsTest, LbuZeroExtendsByte) {
    EXPECT_EQ(format_load_value(Op::LBU, 0xff), 0x000000ff);
}

TEST(OpsTest, LhSignExtendsNegativeHalfword) {
    // raw halfword 0xffff (== -1 as int16_t) sign-extends to 0xffffffff
    EXPECT_EQ(format_load_value(Op::LH, 0xffff), 0xffffffff);
}

TEST(OpsTest, LhSignExtendsPositiveHalfword) {
    EXPECT_EQ(format_load_value(Op::LH, 0x7fff), 0x00007fff);
}

TEST(OpsTest, LhuZeroExtendsHalfword) {
    EXPECT_EQ(format_load_value(Op::LHU, 0xffff), 0x0000ffff);
}

TEST(OpsTest, LwPassesThroughUnchanged) {
    EXPECT_EQ(format_load_value(Op::LW, 0xd3bad3ba), 0xd3bad3ba);
}

TEST(OpsTest, LbIgnoresUpperBitsOfRawValue) {
    // Only the low byte should matter, even if raw_value has garbage
    // in higher bytes (e.g. a caller passing a full word by mistake).
    EXPECT_EQ(format_load_value(Op::LB, 0xabcdef7f), 0x0000007fu);
}

TEST(OpsTest, FormatLoadValueRejectsOutOfFamilyOp) {
    EXPECT_THROW(format_load_value(Op::SB, 0), std::invalid_argument);
    EXPECT_THROW(format_load_value(Op::ADD, 0), std::invalid_argument);
}
