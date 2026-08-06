#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "isa/arf.hpp"
#include "isa/csrf.hpp"
#include "isa/flat_memory.hpp"
#include "isa/functional.hpp"
#include "isa/instruction.hpp"
#include "isa/op.hpp"

using namespace pebble::isa;

class FunctionalExecuteTest : public ::testing::Test {
protected:
    ArchRegisterFile regs{};
    CsrFile csrf{};
    FlatMemory mem{64};
};

namespace {

constexpr uint16_t kTestCsr = 0x1;  // an arbitrary non-counter CSR address

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

Instruction make_csr_reg(Op op, RegId rd, RegId rs1, uint16_t csr_addr) {
    Instruction inst;
    inst.op = op;
    inst.op_fam = OpFamily::Csr;
    inst.rd = rd;
    inst.rs1 = rs1;
    inst.csr_addr = csr_addr;
    return inst;
}

Instruction make_csr_imm(Op op, RegId rd, uint32_t uimm, uint16_t csr_addr) {
    Instruction inst;
    inst.op = op;
    inst.op_fam = OpFamily::Csr;
    inst.rd = rd;
    inst.imm = static_cast<int32_t>(uimm);
    inst.csr_addr = csr_addr;
    return inst;
}

}  // namespace

TEST_F(FunctionalExecuteTest, RegRegAddDispatchesCorrectly) {
    regs.write(RegId{1}, 3);
    regs.write(RegId{2}, 4);

    auto instr = make_reg_reg(Op::ADD, RegId{3}, RegId{1}, RegId{2});
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId{3});
    EXPECT_EQ(r.writeback_value, 7);
}

TEST_F(FunctionalExecuteTest, RegImmAddiDispatchesCorrectly) {
    regs.write(RegId{1}, 10);

    auto instr = make_reg_imm(Op::ADDI, RegId{2}, RegId{1}, 5);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_EQ(r.writeback_value, 15);
}

TEST_F(FunctionalExecuteTest, RegRegAndRegImmNeverSetStoreOrNextPc) {
    regs.write(RegId{1}, 1);
    regs.write(RegId{2}, 2);

    auto instr = make_reg_reg(Op::ADD, RegId{3}, RegId{1}, RegId{2});
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.mem_addr.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, Lui) {
    auto instr = make_upp_imm(Op::LUI, RegId{1}, 0x12345000);
    auto r = functional::execute(instr, 0x1000, regs, csrf);

    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId{1});
    EXPECT_EQ(r.writeback_value, 0x12345000);
}

TEST_F(FunctionalExecuteTest, Auipc) {
    auto instr = make_upp_imm(Op::AUIPC, RegId{1}, 0x2000);
    auto r = functional::execute(instr, 0x1000, regs, csrf);

    EXPECT_EQ(r.writeback_value, 0x3000);
}

TEST_F(FunctionalExecuteTest, BranchTakenSetsNextPc) {
    regs.write(RegId{1}, 5);
    regs.write(RegId{2}, 5);

    auto instr = make_branch(Op::BEQ, RegId{1}, RegId{2}, 8);
    auto r = functional::execute(instr, 100, regs, csrf);

    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 108);
}

TEST_F(FunctionalExecuteTest, BranchNotTakenLeavesNextPcEmpty) {
    regs.write(RegId{1}, 5);
    regs.write(RegId{2}, 6);

    auto instr = make_branch(Op::BEQ, RegId{1}, RegId{2}, 8);
    auto r = functional::execute(instr, 100, regs, csrf);

    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, BranchNeverSetsRdOrWriteback) {
    regs.write(RegId{1}, 1);
    regs.write(RegId{2}, 1);

    auto instr = make_branch(Op::BEQ, RegId{1}, RegId{2}, 4);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.rd.has_value());
}

TEST_F(FunctionalExecuteTest, JalSetsLinkAndTarget) {
    auto instr = make_jal(RegId{1}, 256);
    auto r = functional::execute(instr, 1000, regs, csrf);

    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId{1});
    EXPECT_EQ(r.writeback_value, 1004);  // return address = pc + 4
    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 1256);
}

TEST_F(FunctionalExecuteTest, JalrSetsLinkAndTargetFromRegister) {
    regs.write(RegId{2}, 2000);

    auto instr = make_jalr(RegId{1}, RegId{2}, 8);
    auto r = functional::execute(instr, 1000, regs, csrf);

    EXPECT_EQ(r.writeback_value, 1004);
    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 2008);
}

TEST_F(FunctionalExecuteTest, JalrClearsLsbOfTarget) {
    regs.write(RegId{2}, 2001);  // odd base

    auto instr = make_jalr(RegId{1}, RegId{2}, 0);
    auto r = functional::execute(instr, 1000, regs, csrf);

    ASSERT_TRUE(r.next_pc.has_value());
    EXPECT_EQ(*r.next_pc, 2000);  // lsb cleared
}

TEST_F(FunctionalExecuteTest, LoadComputesAddressButDoesNotRead) {
    regs.write(RegId{1}, 100);

    auto instr = make_load(Op::LB, RegId{2}, RegId{1}, 4);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 104);
    EXPECT_EQ(r.writeback_value, 0);
}

TEST_F(FunctionalExecuteTest, StoreComputesAddressAndValueButDoesNotWrite) {
    regs.write(RegId{1}, 0x10);
    regs.write(RegId{2}, 0xabcd1234);

    auto instr = make_store(Op::SW, RegId{1}, RegId{2}, 4);
    auto r = functional::execute(instr, 0, regs, csrf);

    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 0x14);
    EXPECT_EQ(r.store_value, 0xabcd1234);

    // deferred: memory must be untouched by functional::execute() itself.
    EXPECT_EQ(mem.read(0x14, MemWidth::Word).value, 0u);
}

TEST_F(FunctionalExecuteTest, StoreNeverSetsRdOrNextPc) {
    regs.write(RegId{1}, 0);
    regs.write(RegId{2}, 1);

    auto instr = make_store(Op::SB, RegId{1}, RegId{2}, 0);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.rd.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, StoreNeverTrapsAtExecuteTime) {
    // even a store to an obviously out-of-range address must not trap here; the trap can only surface at commit
    regs.write(RegId{1}, 100000);
    regs.write(RegId{2}, 0);

    auto instr = make_store(Op::SB, RegId{1}, RegId{2}, 0);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.mem_addr.has_value());
    EXPECT_EQ(*r.mem_addr, 100000);
}

TEST_F(FunctionalExecuteTest, FenceIsANoOp) {
    auto instr = make_system(Op::FENCE);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_FALSE(r.trap.is_trap());
    EXPECT_FALSE(r.rd.has_value());
    EXPECT_FALSE(r.next_pc.has_value());
}

TEST_F(FunctionalExecuteTest, EcallProducesEnvironmentCallTrap) {
    auto instr = make_system(Op::ECALL);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_TRUE(r.trap.is_trap());
    EXPECT_EQ(r.trap.kind, TrapKind::EnvironmentCallFromMMode);
}

TEST_F(FunctionalExecuteTest, EbreakProducesBreakpointTrap) {
    auto instr = make_system(Op::EBREAK);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_TRUE(r.trap.is_trap());
    EXPECT_EQ(r.trap.kind, TrapKind::Breakpoint);
}

TEST_F(FunctionalExecuteTest, IllegalFamilyThrowsRatherThanReachingExecute) {
    Instruction instr{};
    instr.op = Op::ILLEGAL;
    instr.op_fam = OpFamily::Illegal;

    EXPECT_THROW(functional::execute(instr, 0, regs, csrf), std::invalid_argument);
}

TEST_F(FunctionalExecuteTest, CsrrwProducesOldValueAsWritebackAndNewValueAsCsrWrite) {
    csrf.write(kTestCsr, 0x1);
    regs.write(RegId{2}, 0x99);

    auto inst = make_csr_reg(Op::CSRRW, RegId{1}, RegId{2}, kTestCsr);
    auto r = functional::execute(inst, 0, regs, csrf);

    EXPECT_FALSE(r.trap.is_trap());
    ASSERT_TRUE(r.rd.has_value());
    EXPECT_EQ(*r.rd, RegId{1});
    EXPECT_EQ(r.writeback_value, 0x1);  // rd gets the OLD csr value

    ASSERT_TRUE(r.csr_addr.has_value());
    EXPECT_EQ(*r.csr_addr, kTestCsr);
    EXPECT_EQ(r.csr_value, 0x99);  // csr gets rs1's value (unconditional overwrite)
}

TEST_F(FunctionalExecuteTest, CsrrwDoesNotMutateCsrFileDirectlyDeferredToCommit) {
    csrf.write(kTestCsr, 0x1);
    regs.write(RegId{2}, 0x99);

    auto instr = make_csr_reg(Op::CSRRW, RegId{1}, RegId{2}, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);

    // execute() must not have applied the commit itself: only FunctionalCpu::step() commits it
    EXPECT_EQ(csrf.read(kTestCsr), 0x1);
}

TEST_F(FunctionalExecuteTest, CsrrsSetsBits) {
    csrf.write(kTestCsr, 0b1010);
    regs.write(RegId{2}, 0b0101);

    auto instr = make_csr_reg(Op::CSRRS, RegId{1}, RegId{2}, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_EQ(r.writeback_value, 0b1010);  // old value
    EXPECT_EQ(r.csr_value, 0b1111);  // old value | operand
}

TEST_F(FunctionalExecuteTest, CsrrcClearsBits) {
    csrf.write(kTestCsr, 0b1111);
    regs.write(RegId{2}, 0b0101);

    auto instr = make_csr_reg(Op::CSRRC, RegId{1}, RegId{2}, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_EQ(r.writeback_value, 0b1111);  // old value
    EXPECT_EQ(r.csr_value, 0b1010);  // old value & ~operand
}

TEST_F(FunctionalExecuteTest, CsrrwiUsesImmediateNotRegister) {
    csrf.write(kTestCsr, 0x7);
    /* rs1 is intentionally left unset (nullopt) -- *i variants never read a register; using an
     * uninitialized/never-written RegId would crash if execute_csr mistakenly tried regs.read(*inst.rs1) */
    auto instr = make_csr_imm(Op::CSRRWI, RegId{1}, 0b10101, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);

    EXPECT_EQ(r.writeback_value, 0x7u);  // old value
    EXPECT_EQ(r.csr_value, 0b10101u);  // uimm, unconditional overwrite
}

TEST_F(FunctionalExecuteTest, CsrrsiSetsBitsFromImmediate) {
    csrf.write(kTestCsr, 0b1000);
    auto instr = make_csr_imm(Op::CSRRSI, RegId{1}, 0b0011, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);
    
    EXPECT_EQ(r.csr_value, 0b1011u);
}

TEST_F(FunctionalExecuteTest, CsrrciClearsBitsFromImmediate) {
    csrf.write(kTestCsr, 0b1111);
    auto instr = make_csr_imm(Op::CSRRCI, RegId{1}, 0b0011, kTestCsr);
    auto r = functional::execute(instr, 0, regs, csrf);
    
    EXPECT_EQ(r.csr_value, 0b1100u);
}

TEST_F(FunctionalExecuteTest, ReadingCycleCsrReflectsCsrFileCounter) {
    csrf.increment_cycle(12345);
    auto instr = make_csr_reg(Op::CSRRS, RegId{1}, RegId{0}, CsrIndex::kCycleLo);
    // rs1 = x0 -> operand 0 -> Csrrs with 0 is a pure read, no bits set.
    regs.write(RegId{0}, 0);
    auto r = functional::execute(instr, 0, regs, csrf);
    
    EXPECT_EQ(r.writeback_value, 12345u);
}
