#include <cstdint>
#include <stdexcept>
#include "isa/ops.hpp"
#include "utils/cast.hpp"

namespace pebble::isa {
namespace {
using Cast = utils::Cast;
}  // namespace

uint32_t compute_reg_reg(Op op, uint32_t rs1_val, uint32_t rs2_val) {
    /* needed when upcasting to int64_t to preserve sign -- direct upcasting uint32_t to int64_t
     * leads to zero-extending instead of sign-extending */
    const int32_t rs1_s = Cast::i32(rs1_val);
    const int32_t rs2_s = Cast::i32(rs2_val);

    uint64_t pu = 0;
    int64_t ps = 0;

    switch(op) {
        case Op::ADD:  return rs1_val + rs2_val;
        case Op::SUB:  return rs1_val - rs2_val;  // wraparound case is not our concern
        case Op::XOR:  return rs1_val ^ rs2_val;
        case Op::OR:   return rs1_val | rs2_val;
        case Op::AND:  return rs1_val & rs2_val;
        case Op::SLL:  return rs1_val << (rs2_val & 0x1f);  // lower 5-bits used for left shift
        case Op::SRL:  return rs1_val >> (rs2_val & 0x1f);  // lower 5-bits used for right shift
        case Op::SRA:  return Cast::u32(rs1_s >> (rs2_val & 0x1f));
        case Op::SLT:  return (rs1_s < rs2_s)? 1: 0;
        case Op::SLTU: return (rs1_val < rs2_val)? 1: 0;

        /* M-extension */
        case Op::MUL:
            return Cast::u32(rs1_val * rs2_val);

        case Op::MULH:
            // rs1 sign-extended, rs2 sign-extended
            pu = Cast::u64(Cast::i64(rs1_s) * Cast::i64(rs2_s));
            return Cast::u32(pu >> 32);

        case Op::MULHSU:
            // rs1 sign-extended, rs2 zero-extended (not sign-extended!)
            ps = Cast::i64(rs1_s) * Cast::i64(Cast::u64(rs2_val));
            return Cast::u32(Cast::u64(ps) >> 32);

        case Op::MULHU:
            // rs1 zero-extended, rs2 zero-extended
            pu = Cast::u64(rs1_val) * Cast::u64(rs2_val);
            return Cast::u32(pu >> 32);

        case Op::DIV:
            // RISC-V spec-mandated results for edge cases (no UB)
            if(rs2_s == 0) return 0xffffffff;  // case: division by 0
            if(rs1_s == INT32_MIN && rs2_s == -1) return rs1_val;  // case: overflow
            return Cast::u32(rs1_s / rs2_s);

        case Op::DIVU:
            if(rs2_val == 0) return 0xffffffff;  // case: division by 0
            return rs1_val / rs2_val;

        case Op::REM:
            if(rs2_s == 0) return rs1_val;  // case: modulo by 0
            if(rs1_s == INT32_MIN && rs2_s == -1) return 0;  // case: overflow
            return Cast::u32(rs1_s % rs2_s);

        case Op::REMU:
            if(rs2_val == 0) return rs1_val;  // case: modulo by 0
            return rs1_val % rs2_val;

        default: throw std::invalid_argument{"invalid op provided for reg-reg computation"};
    }
}

uint32_t compute_reg_imm(Op op, uint32_t rs1_val, int32_t imm) {
    const int32_t rs1_s = Cast::i32(rs1_val);
    const uint32_t imm_u = Cast::u32(imm);

    // only SLLI/SRLI/SRAI carry a shamt (shift amount) in imm (decoder places it int32_t)
    const uint32_t shamt = imm_u & 0x1f;

    switch(op) {
        case Op::ADDI:  return rs1_val + imm_u;
        case Op::XORI:  return rs1_val ^ imm_u;
        case Op::ORI:   return rs1_val | imm_u;
        case Op::ANDI:  return rs1_val & imm_u;
        case Op::SLLI:  return rs1_val << shamt;
        case Op::SRLI:  return rs1_val >> shamt;
        case Op::SRAI:  return Cast::u32(rs1_s >> shamt);
        case Op::SLTI:  return (rs1_s < imm)? 1: 0;
        case Op::SLTIU: return (rs1_val < imm_u)? 1: 0;
        default: throw std::invalid_argument{"invalid op provided for reg-imm computation"};
    }
}

bool compute_branch_taken(Op op, uint32_t rs1_val, uint32_t rs2_val) {
    const int32_t rs1_s = Cast::i32(rs1_val);
    const int32_t rs2_s = Cast::i32(rs2_val);

    switch(op) {
        case Op::BEQ:  return rs1_val == rs2_val;
        case Op::BNE:  return rs1_val != rs2_val;
        case Op::BLT:  return rs1_s < rs2_s;
        case Op::BGE:  return rs1_s >= rs2_s;
        case Op::BLTU: return rs1_val < rs2_val;
        case Op::BGEU: return rs1_val >= rs2_val;
        default: throw std::invalid_argument{"invalid op provided for branch computation"};
    }
}

uint32_t compute_upp_imm(Op op, uint32_t pc, int32_t imm) {
    switch(op) {
        case Op::LUI:   return Cast::u32(imm);  // no need to shift by 12; decoder already did that (by masking out 0...11)
        case Op::AUIPC: return pc + Cast::u32(imm);  // same thing as Cast::u32(pc + imm) -> imm is implicitly upcasted to uint32_t
        default: throw std::invalid_argument{"invalid op provided for upp-imm computation"};
    }
}

std::uint32_t format_load_value(Op op, uint32_t raw_val) {
    switch(op) {
        case Op::LB:  return Cast::u32(Cast::i32(Cast::i8(raw_val & 0xff)));
        case Op::LH:  return Cast::u32(Cast::i32(Cast::i16(raw_val & 0xffff)));
        case Op::LW:  return raw_val;  // full word; no extension is needed
        case Op::LBU: return raw_val & 0xff;
        case Op::LHU: return raw_val & 0xffff;
        default: throw std::invalid_argument{"invalid op provided for formatting load value"};
    }
}

}  // namespace pebble::isa