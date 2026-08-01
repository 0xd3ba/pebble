#pragma once

#include <cstdint>
#include "isa/op.hpp"

namespace pebble::isa {

/* Pure, side-effect-free RISC-V semantics shared by every CPU model.
 * No RegId, no ArchRegisterFile, no Instruction, no memory access -- just raw values in, computed value out */
[[nodiscard]] uint32_t compute_reg_reg(Op op, uint32_t rs1_val, uint32_t rs2_val);
[[nodiscard]] uint32_t compute_reg_imm(Op op, uint32_t rs1_val, int32_t imm);
[[nodiscard]] bool compute_branch_taken(Op op, uint32_t rs1_val, uint32_t rs2_val);
[[nodiscard]] uint32_t compute_upp_imm(Op op, uint32_t pc, int32_t imm);

/* Load-result formatting (sign/zero-extension based on width and signedness) -- applied to raw bytes already
 * read from memory by whatever model-specific mechanism did the actual access */
[[nodiscard]] std::uint32_t format_load_value(Op op, uint32_t raw_val);

}  // namespace pebble::isa