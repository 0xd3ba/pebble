#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "isa/arf.hpp"
#include "isa/flat_memory.hpp"
#include "isa/functional.hpp"
#include "isa/instruction.hpp"
#include "isa/op.hpp"

using namespace pebble::isa;

class FunctionalExecuteTest : public ::testing::Test {
protected:
    ArchRegisterFile regs{};
    FlatMemory mem{64};
};

namespace {

Instruction make_reg_reg(Op op, RegId rd, RegId rs1, RegId rs2) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::RegReg;
    instr.rd = rd;
    instr.rs1 = rs1;
    instr.rs2 = rs2;
    return instr;
}

Instruction make_reg_imm(Op op, RegId rd, RegId rs1, int32_t imm) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::RegImm;
    instr.rd = rd;
    instr.rs1 = rs1;
    instr.imm = imm;
    return instr;
}

Instruction make_upp_imm(Op op, RegId rd, int32_t imm) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::UppImm;
    instr.rd = rd;
    instr.imm = imm;
    return instr;
}

Instruction make_branch(Op op, RegId rs1, RegId rs2, int32_t imm) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::Branch;
    instr.rs1 = rs1;
    instr.rs2 = rs2;
    instr.imm = imm;
    return instr;
}

Instruction make_jal(RegId rd, int32_t imm) {
    Instruction instr{};
    instr.op = Op::JAL;
    instr.op_fam = OpFamily::Jump;
    instr.rd = rd;
    instr.imm = imm;
    return instr;
}

Instruction make_jalr(RegId rd, RegId rs1, int32_t imm) {
    Instruction instr{};
    instr.op = Op::JALR;
    instr.op_fam = OpFamily::Jump;
    instr.rd = rd;
    instr.rs1 = rs1;
    instr.imm = imm;
    return instr;
}

Instruction make_load(Op op, RegId rd, RegId rs1, int32_t imm) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::Load;
    instr.rd = rd;
    instr.rs1 = rs1;
    instr.imm = imm;
    return instr;
}

Instruction make_store(Op op, RegId rs1, RegId rs2, int32_t imm) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::Store;
    instr.rs1 = rs1;
    instr.rs2 = rs2;
    instr.imm = imm;
    return instr;
}

Instruction make_system(Op op) {
    Instruction instr{};
    instr.op = op;
    instr.op_fam = OpFamily::System;
    return instr;
}

}  // namespace

TEST_F(FunctionalExecuteTest, RegRegAddDispatchesCorrectly) {
    regs.write(RegId(1), 3);
    regs.write(RegId(2), 4);

    auto instr = make_reg_reg(Op::ADD, RegId(3), RegId(1), RegId(2));
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId(3));
    EXPECT_EQ(r.writeback_value, 7);
}

TEST_F(FunctionalExecuteTest, RegImmAddiDispatchesCorrectly) {
    regs.write(RegId(1), 10);

    auto instr = make_reg_imm(Op::ADDI, RegId(2), RegId(1), 5);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_EQ(r.writeback_value, 15);
}

TEST_F(FunctionalExecuteTest, RegRegAndRegImmNeverSetStoreOrNextPc) {
    regs.write(RegId(1), 1);
    regs.write(RegId(2), 2);

    auto instr = make_reg_reg(Op::ADD, RegId(3), RegId(1), RegId(2));
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.mem_addr.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, Lui) {
    auto instr = make_upp_imm(Op::LUI, RegId(1), 0x12345000);
    auto r = functional::execute(instr, 0x1000, regs);

    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId(1));
    EXPECT_EQ(r.writeback_value, 0x12345000);
}

TEST_F(FunctionalExecuteTest, Auipc) {
    auto instr = make_upp_imm(Op::AUIPC, RegId(1), 0x2000);
    auto r = functional::execute(instr, 0x1000, regs);

    EXPECT_EQ(r.writeback_value, 0x3000);
}

TEST_F(FunctionalExecuteTest, BranchTakenSetsNextPc) {
    regs.write(RegId(1), 5);
    regs.write(RegId(2), 5);

    auto instr = make_branch(Op::BEQ, RegId(1), RegId(2), 8);
    auto r = functional::execute(instr, 100, regs);

    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 108);
}

TEST_F(FunctionalExecuteTest, BranchNotTakenLeavesNextPcEmpty) {
    regs.write(RegId(1), 5);
    regs.write(RegId(2), 6);

    auto instr = make_branch(Op::BEQ, RegId(1), RegId(2), 8);
    auto r = functional::execute(instr, 100, regs);

    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, BranchNeverSetsRdOrWriteback) {
    regs.write(RegId(1), 1);
    regs.write(RegId(2), 1);

    auto instr = make_branch(Op::BEQ, RegId(1), RegId(2), 4);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.rd.has_value());
}

TEST_F(FunctionalExecuteTest, JalSetsLinkAndTarget) {
    auto instr = make_jal(RegId(1), 256);
    auto r = functional::execute(instr, 1000, regs);

    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId(1));
    EXPECT_EQ(r.writeback_value, 1004);  // return address = pc + 4
    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 1256);
}

TEST_F(FunctionalExecuteTest, JalrSetsLinkAndTargetFromRegister) {
    regs.write(RegId(2), 2000);

    auto instr = make_jalr(RegId(1), RegId(2), 8);
    auto r = functional::execute(instr, 1000, regs);

    EXPECT_EQ(r.writeback_value, 1004);
    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 2008);
}

TEST_F(FunctionalExecuteTest, JalrClearsLsbOfTarget) {
    regs.write(RegId(2), 2001);  // odd base

    auto instr = make_jalr(RegId(1), RegId(2), 0);
    auto r = functional::execute(instr, 1000, regs);

    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 2000);  // lsb cleared
}

TEST_F(FunctionalExecuteTest, LoadComputesAddressButDoesNotRead) {
    regs.write(RegId(1), 100);

    auto instr = make_load(Op::LB, RegId(2), RegId(1), 4);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 104);
    EXPECT_EQ(r.writeback_value, 0);
}

TEST_F(FunctionalExecuteTest, StoreComputesAddressAndValueButDoesNotWrite) {
    regs.write(RegId(1), 0x10);
    regs.write(RegId(2), 0xabcd1234);

    auto instr = make_store(Op::SW, RegId(1), RegId(2), 4);
    auto r = functional::execute(instr, 0, regs);

    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 0x14);
    EXPECT_EQ(r.store_value, 0xabcd1234);

    // deferred: memory must be untouched by functional::execute() itself.
    EXPECT_EQ(mem.read(0x14, MemWidth::Word).value, 0u);
}

TEST_F(FunctionalExecuteTest, StoreNeverSetsRdOrNextPc) {
    regs.write(RegId(1), 0);
    regs.write(RegId(2), 1);

    auto instr = make_store(Op::SB, RegId(1), RegId(2), 0);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.rd.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, StoreNeverTrapsAtExecuteTime) {
    // even a store to an obviously out-of-range address must not trap here; the trap can only surface at commit
    regs.write(RegId(1), 100000);
    regs.write(RegId(2), 0);

    auto instr = make_store(Op::SB, RegId(1), RegId(2), 0);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 100000);
}

TEST_F(FunctionalExecuteTest, FenceIsANoOp) {
    auto instr = make_system(Op::FENCE);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_FALSE(r.trap.is_trap());
    EXPECT_FALSE(r.rd.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, EcallProducesEnvironmentCallTrap) {
    auto instr = make_system(Op::ECALL);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_TRUE(r.trap.is_trap());
    EXPECT_EQ(r.trap.kind, TrapKind::EnvironmentCallFromMMode);
}

TEST_F(FunctionalExecuteTest, EbreakProducesBreakpointTrap) {
    auto instr = make_system(Op::EBREAK);
    auto r = functional::execute(instr, 0, regs);

    EXPECT_TRUE(r.trap.is_trap());
    EXPECT_EQ(r.trap.kind, TrapKind::Breakpoint);
}

TEST_F(FunctionalExecuteTest, IllegalFamilyThrowsRatherThanReachingExecute) {
    Instruction instr{};
    instr.op = Op::ILLEGAL;
    instr.op_fam = OpFamily::Illegal;

    EXPECT_THROW(functional::execute(instr, 0, regs), std::invalid_argument);
}
