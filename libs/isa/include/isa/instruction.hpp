#pragma once

#include <cstdint>
#include <optional>
#include "isa/op.hpp"
#include "isa/reg_id.hpp"

namespace pebble::isa {

using word_t = uint32_t;

/* Instruction -- a decoded RV32I+M instruction word. Flat struct across all formats for easy representation
 * (reducing few bytes per instruction is not worth the complexity involved in multi-format instructions) */
struct Instruction {
    word_t raw{0};                       // Raw instruction word (undecoded)
    Op op{Op::ILLEGAL};                  // Instruction identifier
    OpFamily op_fam{OpFamily::Illegal};  // Instruction family

    std::optional<RegId> rd{};   // Destination register ID
    std::optional<RegId> rs1{};  // Source register #1 ID
    std::optional<RegId> rs2{};  // Source register #2 ID
    word_t imm{0};               // Immediate field (will be sign-extended by the decoder)
    uint8_t funct3{0};           // ID mapping a family of instructions (for e.g. register-register)
    uint8_t funct7{0};           // ID to disambiguate between each instruction within a given family (for e.g. add vs. sub)

    bool is_illegal() const noexcept { return op == Op::ILLEGAL; }
};

}  // namespace pebble::isa