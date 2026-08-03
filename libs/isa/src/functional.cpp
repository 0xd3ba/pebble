#include <cstdint>
#include <stdexcept>
#include <utility>
#include "isa/addr.hpp"
#include "isa/functional.hpp"
#include "isa/mem_width.hpp"
#include "isa/op.hpp"
#include "isa/ops.hpp"
#include "utils/cast.hpp"

namespace pebble::isa::functional {

FunctionalExecutionResult execute(const Instruction &instr, addr_t pc, const ArchRegisterFile &regs, CsrFile &csrf) {
    const uint32_t rs1_val = instr.rs1.has_value()? regs.read(*instr.rs1): 0;
    const uint32_t rs2_val = instr.rs2.has_value()? regs.read(*instr.rs2): 0;
    FunctionalExecutionResult r{};
    r.rd = instr.rd;

    switch(instr.op_fam) {
        case OpFamily::RegReg:
            r.writeback_value = compute_reg_reg(instr.op, rs1_val, rs2_val);
            break;

        case OpFamily::RegImm:
            r.writeback_value = compute_reg_imm(instr.op, rs1_val, instr.imm);
            break;

        case OpFamily::Load:
            r.mem_addr = rs1_val + utils::Cast::u32(instr.imm);
            // read deferred until next stage (mem-access) by the functional cpu -- misaligned/out-of-range read only detected later
            break;

        case OpFamily::Store:
            r.mem_addr = rs1_val + utils::Cast::u32(instr.imm);
            r.store_value = rs2_val;
            // write deferred until commit/writeback by the functional cpu -- misaligned/out-of-range write only detected later
            break;

        case OpFamily::Branch:
            if(compute_branch_taken(instr.op, rs1_val, rs2_val))
                r.next_pc = pc + utils::Cast::u32(instr.imm);
            break;

        case OpFamily::Jump: {
            r.writeback_value = pc + 4;  // return address in rd
            switch(instr.op) {
                case Op::JAL:
                    r.next_pc = pc + utils::Cast::u32(instr.imm);
                    break;

                case Op::JALR:
                    // according to risc-v spec, target address's LSB (bit 0) is cleared
                    r.next_pc = (rs1_val + utils::Cast::u32(instr.imm)) & ~utils::Cast::u32(1);
                    break;

                default: throw std::invalid_argument{"execute() on jump family but op unsupported"};
            }
            break;
        }

        case OpFamily::UppImm:
            r.writeback_value = compute_upp_imm(instr.op, pc, instr.imm);
            break;

        case OpFamily::System: {
            switch(instr.op) {
                case Op::FENCE:
                    break;  // no-op; single-hart, no mem-ordering hazard to enforce

                case Op::ECALL:
                  r.trap.kind = TrapKind::EnvironmentCallFromMMode;
                  break;

                case Op::EBREAK:
                    r.trap.kind = TrapKind::Breakpoint;
                    break;

                default: throw std::invalid_argument{"execute() on system family but op unsupported"};
            }
            break;
        }

        case OpFamily::Csr: {
            uint32_t old_csr_val = csrf.read(instr.csr_addr);
            uint32_t operand = instr.rs1.has_value()? regs.read(*instr.rs1): utils::Cast::u32(instr.imm);  // *i variants use imm (rs1 unset)
            uint32_t new_csr_val = compute_csr_write(instr.op, old_csr_val, operand);

            r.writeback_value = old_csr_val;  // old value is stored in rd
            r.csr_addr = instr.csr_addr;
            r.csr_value = new_csr_val;
            break;
        }

        case OpFamily::Illegal:
            throw std::invalid_argument{"execute() called with illegal instruction; functional cpu must not route them here"};

        default: throw std::invalid_argument{"execute(): unhandled op family received"};
    }

    // for branch/jump instructions, need to ensure that the next PC is aligned to word boundary (4 byte alignment)
    if(r.next_pc.has_value()) {
        if(*r.next_pc & 0x3) {
            Trap t{};
            t.kind = TrapKind::PCAddressMisaligned;
            t.faulting_addr = *r.next_pc;
            t.message = "PC address misaligned";
            r.trap = std::move(t);
        }
    }

    return r;
}

}  // namespace pebble::isa