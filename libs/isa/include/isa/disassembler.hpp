#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <stdexcept>
#include "isa/instruction.hpp"
#include "isa/op.hpp"
#include "isa/reg_id.hpp"

namespace pebble::isa::debug {

/* Disassembler -- formats a decoded Instruction as a human-readable RISC-V assembly string.
 * For debugging/trace-dump purposes only */
class Disassembler {
public:
    static std::string disassemble(const Instruction &instr);

private:
    static std::string reg_name(const std::optional<RegId> &r) {
        return r.has_value()? "x" + std::to_string(r->index()): "?";
    }

    static std::string mnemonic(Op op) {
        switch(op) {
            case Op::ADD:    return "add";
            case Op::SUB:    return "sub";
            case Op::XOR:    return "xor";
            case Op::OR:     return "or";
            case Op::AND:    return "and";
            case Op::SLL:    return "sll";
            case Op::SRL:    return "srl";
            case Op::SRA:    return "sra";
            case Op::SLT:    return "slt";
            case Op::SLTU:   return "sltu";

            case Op::ADDI:   return "addi";
            case Op::XORI:   return "xori";
            case Op::ORI:    return "ori";
            case Op::ANDI:   return "andi";
            case Op::SLLI:   return "slli";
            case Op::SRLI:   return "srli";
            case Op::SRAI:   return "srai";
            case Op::SLTI:   return "slti";
            case Op::SLTIU:  return "sltiu";

            case Op::LB:     return "lb";
            case Op::LH:     return "lh";
            case Op::LW:     return "lw";
            case Op::LBU:    return "lbu";
            case Op::LHU:    return "lhu";
            case Op::SB:     return "sb";
            case Op::SH:     return "sh";
            case Op::SW:     return "sw";

            case Op::BEQ:    return "beq";
            case Op::BNE:    return "bne";
            case Op::BLT:    return "blt";
            case Op::BGE:    return "bge";
            case Op::BLTU:   return "bltu";
            case Op::BGEU:   return "bgeu";
            case Op::JAL:    return "jal";
            case Op::JALR:   return "jalr";

            case Op::LUI:    return "lui";
            case Op::AUIPC:  return "auipc";

            case Op::FENCE:  return "fence";
            case Op::ECALL:  return "ecall";
            case Op::EBREAK: return "ebreak";
            case Op::MRET:   return "mret";
            case Op::CSRRW:  return "csrrw";
            case Op::CSRRS:  return "csrrs";
            case Op::CSRRC:  return "csrrc";
            case Op::CSRRWI: return "csrrwi";
            case Op::CSRRSI: return "csrrsi";
            case Op::CSRRCI: return "csrrci";

            case Op::MUL:    return "mul";
            case Op::MULH:   return "mulh";
            case Op::MULHSU: return "mulhsu";
            case Op::MULHU:  return "mulhu";
            case Op::DIV:    return "div";
            case Op::DIVU:   return "divu";
            case Op::REM:    return "rem";
            case Op::REMU:   return "remu";
            case Op::ILLEGAL: return "illegal";
        }

        return "[unknown]";  // no point in throwing while we are debugging
    }
};

}  // namespace pebble::isa::debug