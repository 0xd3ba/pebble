#include <gtest/gtest.h>
#include "isa/decoder.hpp"
#include "isa/instruction.hpp"
#include "isa/op.hpp"
#include "isa/reg_id.hpp"

/* Note: Each word below is hand-encoded from the RV32I/M specification (not generated via an assembler) */
using namespace pebble::isa;

TEST(DecoderTest, Addi) {
    // addi x1, x0, 5
    Instruction inst = Decoder::decode(0x00500093);
    EXPECT_EQ(inst.op, Op::ADDI);
    EXPECT_EQ(inst.op_fam, OpFamily::RegImm);
    ASSERT_TRUE(inst.rd.has_value());
    EXPECT_EQ(*inst.rd, RegId(1));
    ASSERT_TRUE(inst.rs1.has_value());
    EXPECT_EQ(*inst.rs1, RegId(0));
    EXPECT_FALSE(inst.rs2.has_value());
    EXPECT_EQ(inst.imm, 5);
    EXPECT_FALSE(inst.is_illegal());
}

TEST(DecoderTest, AddiNegativeImmediateSignExtends) {
    // addi x1, x0, -1
    Instruction inst = Decoder::decode(0xFFF00093);
    EXPECT_EQ(inst.op, Op::ADDI);
    EXPECT_EQ(inst.imm, -1);
}

TEST(DecoderTest, AddiZeroZeroZeroIsNop) {
    // addi x0, x0, 0  (canonical no-op)
    Instruction inst = Decoder::decode(0x00000013);
    EXPECT_EQ(inst.op, Op::ADDI);
    ASSERT_TRUE(inst.rd.has_value());
    EXPECT_EQ(*inst.rd, RegId(0));
    EXPECT_EQ(inst.imm, 0);
}

TEST(DecoderTest, SlliFunct7NonZeroIsIllegal) {
    // slli requires funct7 == 0; corrupt it -> illegal
    // addi-shaped word with funct3=001 (slli) and funct7=0100000
    Instruction inst = Decoder::decode(0x40001093);
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, Add) {
    // add x3, x1, x2
    Instruction inst = Decoder::decode(0x002081B3);
    EXPECT_EQ(inst.op, Op::ADD);
    EXPECT_EQ(inst.op_fam, OpFamily::RegReg);
    EXPECT_EQ(*inst.rd, RegId(3));
    EXPECT_EQ(*inst.rs1, RegId(1));
    EXPECT_EQ(*inst.rs2, RegId(2));
}

TEST(DecoderTest, Sub) {
    // sub x3, x1, x2  (same fields as Add, funct7 = 0100000)
    Instruction inst = Decoder::decode(0x402081B3);
    EXPECT_EQ(inst.op, Op::SUB);
    EXPECT_EQ(inst.op_fam, OpFamily::RegReg);
    EXPECT_EQ(*inst.rd, RegId(3));
    EXPECT_EQ(*inst.rs1, RegId(1));
    EXPECT_EQ(*inst.rs2, RegId(2));
}

TEST(DecoderTest, Mul) {
    // mul x3, x1, x2  (funct7 = 0000001, M extension)
    Instruction inst = Decoder::decode(0x022081B3);
    EXPECT_EQ(inst.op, Op::MUL);
    EXPECT_EQ(inst.op_fam, OpFamily::RegReg);
    EXPECT_EQ(*inst.rd, RegId(3));
    EXPECT_EQ(*inst.rs1, RegId(1));
    EXPECT_EQ(*inst.rs2, RegId(2));
}

TEST(DecoderTest, SllFunct7NonZeroIsIllegal) {
    // sll (funct3=001) requires funct7=0; funct7=0100000 with funct3=001 is not a legal R-type combination.
    Instruction inst = Decoder::decode(0x402090B3);
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, Lw) {
    // lw x5, 8(x2)
    Instruction inst = Decoder::decode(0x00812283);
    EXPECT_EQ(inst.op, Op::LW);
    EXPECT_EQ(inst.op_fam, OpFamily::Load);
    EXPECT_EQ(*inst.rd, RegId(5));
    EXPECT_EQ(*inst.rs1, RegId(2));
    EXPECT_FALSE(inst.rs2.has_value());
    EXPECT_EQ(inst.imm, 8);
}

TEST(DecoderTest, Sw) {
    // sw x5, 8(x2)
    Instruction inst = Decoder::decode(0x00512423);
    EXPECT_EQ(inst.op, Op::SW);
    EXPECT_EQ(inst.op_fam, OpFamily::Store);
    EXPECT_FALSE(inst.rd.has_value());
    EXPECT_EQ(*inst.rs1, RegId(2));
    EXPECT_EQ(*inst.rs2, RegId(5));
    EXPECT_EQ(inst.imm, 8);
}

TEST(DecoderTest, SwNegativeOffsetSignExtends) {
    // sw x5, -8(x2)
    Instruction inst = Decoder::decode(0xFE512C23);
    EXPECT_EQ(inst.op, Op::SW);
    EXPECT_EQ(inst.imm, -8);
}

TEST(DecoderTest, LoadInvalidFunct3IsIllegal) {
    // funct3 = 011 is not a defined load width.
    Instruction inst = Decoder::decode(0x00813383);  // same as Lw word but funct3=011
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, Beq) {
    // beq x1, x2, 8
    Instruction inst = Decoder::decode(0x00208463);
    EXPECT_EQ(inst.op, Op::BEQ);
    EXPECT_EQ(inst.op_fam, OpFamily::Branch);
    EXPECT_EQ(*inst.rs1, RegId(1));
    EXPECT_EQ(*inst.rs2, RegId(2));
    EXPECT_FALSE(inst.rd.has_value());
    EXPECT_EQ(inst.imm, 8);
}

TEST(DecoderTest, BranchInvalidFunct3IsIllegal) {
    // funct3 = 010 is not a defined branch condition.
    Instruction inst = Decoder::decode(0x00202263);  // Beq word with funct3=010
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, Lui) {
    // lui x1, 0x12345
    Instruction inst = Decoder::decode(0x123450B7);
    EXPECT_EQ(inst.op, Op::LUI);
    EXPECT_EQ(inst.op_fam, OpFamily::UppImm);
    EXPECT_EQ(*inst.rd, RegId(1));
    EXPECT_FALSE(inst.rs1.has_value());
    EXPECT_FALSE(inst.rs2.has_value());
    EXPECT_EQ(inst.imm, static_cast<std::int32_t>(0x12345000));
}

TEST(DecoderTest, Jal) {
    // jal x1, 256
    Instruction inst = Decoder::decode(0x100000EF);
    EXPECT_EQ(inst.op, Op::JAL);
    EXPECT_EQ(inst.op_fam, OpFamily::Jump);
    EXPECT_EQ(*inst.rd, RegId(1));
    EXPECT_EQ(inst.imm, 256);
}

TEST(DecoderTest, JalrNegativeOffset) {
    // jalr x1, -4(x2)
    Instruction inst = Decoder::decode(0xFFC100E7);
    EXPECT_EQ(inst.op, Op::JALR);
    EXPECT_EQ(inst.op_fam, OpFamily::Jump);
    EXPECT_EQ(*inst.rd, RegId(1));
    EXPECT_EQ(*inst.rs1, RegId(2));
    EXPECT_EQ(inst.imm, -4);
}

TEST(DecoderTest, JalrInvalidFunct3IsIllegal) {
    // JALR requires funct3 == 0.
    Instruction inst = Decoder::decode(0xFFC110E7);  // same word, funct3=001
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, Ecall) {
    Instruction inst = Decoder::decode(0x00000073);
    EXPECT_EQ(inst.op, Op::ECALL);
    EXPECT_EQ(inst.op_fam, OpFamily::System);
}

TEST(DecoderTest, AllOnesWordIsIllegal) {
    Instruction inst = Decoder::decode(0xFFFFFFFF);
    EXPECT_TRUE(inst.is_illegal());
    EXPECT_EQ(inst.raw, 0xFFFFFFFFu);
}

TEST(DecoderTest, AllZeroWordIsIllegal) {
    // opcode 0000000 doesn't correspond to any defined instruction
    Instruction inst = Decoder::decode(0x00000000);
    EXPECT_EQ(inst.op_fam, OpFamily::Illegal);
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, UnassignedOpcodeIsIllegal) {
    // opcode 1111111 is reserved/unassigned in the base ISA
    Instruction inst = Decoder::decode(0x0000007F);
    EXPECT_EQ(inst.op_fam, OpFamily::Illegal);
    EXPECT_TRUE(inst.is_illegal());
}

TEST(DecoderTest, IllegalInstructionPreservesRawWord) {
    const std::uint32_t garbage = 0xD3BA4400;
    Instruction inst = Decoder::decode(garbage);
    EXPECT_TRUE(inst.is_illegal());
    EXPECT_EQ(inst.raw, garbage);
}
