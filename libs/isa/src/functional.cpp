#include <cstdint>
#include <stdexcept>
#include "isa/addr.hpp"
#include "isa/flat_memory.hpp"
#include "isa/functional.hpp"
#include "isa/mem_width.hpp"
#include "isa/op.hpp"
#include "isa/ops.hpp"
#include "utils/cast.hpp"

namespace pebble::isa::functional {
namespace {

/* Returns the appropriate width of load/store instruction */
MemWidth width_of(Op op) {
    switch (op) {
        case Op::LB:
        case Op::LBU:
        case Op::SB:
            return MemWidth::Byte;

        case Op::LH:
        case Op::LHU:
        case Op::SH:
            return MemWidth::Half;

        case Op::LW:
        case Op::SW:
            return MemWidth::Word;

        default:
            throw std::invalid_argument{"width_of: not a load/store op"};
    }
}

}  // namespace

FunctionalExecutionResult execute(const Instruction &instr, addr_t pc, const ArchRegisterFile &regs, const FlatMemory &mem, [[maybe_unused]] const CsrFile &csrf) {
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

        case OpFamily::Load: {
            addr_t load_addr = rs1_val + utils::Cast::u32(instr.imm);
            flat_memory::ReadResult read_res = mem.read(load_addr, width_of(instr.op));
            if(read_res.trap.is_trap()) {
                r.trap = read_res.trap;
                r.rd = std::nullopt;
            }
            else r.writeback_value = format_load_value(instr.op, read_res.value);
            break;
        }

        case OpFamily::Store:
            r.store_addr = rs1_val + utils::Cast::u32(instr.imm);
            r.store_value = rs2_val;
            // write deferred until commit by the functional cpu -- misaligned/out-of-range write only detected later
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

        case OpFamily::Illegal:
            throw std::invalid_argument{"execute() called with illegal instruction; functional cpu must not route them here"};

        default: throw std::invalid_argument{"execute(): unhandled op family received"};
    }

    return r;
}

}  // namespace pebble::isa