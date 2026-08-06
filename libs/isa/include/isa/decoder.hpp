#pragma once

#include <cstdint>
#include <spdlog/spdlog.h>
#include "isa/instruction.hpp"
#include "isa/reg_id.hpp"
#include "utils/bits.hpp"

namespace pebble::isa {

/* Decoder -- pure, stateless RV32I+M decode: raw instruction word -> structured Instruction.
 * No side effects, no architectural state touched.
 *
 * Note: decode() never throws: an unrecognized encoding is a normal, expected outcome when interpreting arbitrary
 * compiled code. It will be logged at `critical` severity and returned as Instruction{.raw = word, .op = Op::Illegal}
 * data which the caller can use to raise an illegal-instruction exception rather than crashing the simulator
 */
class Decoder {
public:
    [[nodiscard]] static Instruction decode(word_t w) noexcept {
        const uint32_t opcode = w & opcode_mask_;
        switch(opcode) {
            case 0b0110011: return decode_reg_reg(w);               // RV32I: register-register ALU / RV32M: M extension instructions
            case 0b0010011: return decode_reg_imm(w);               // RV32I: register-immediate ALU
            case 0b0000011: return decode_load(w);                  // RV32I: loads
            case 0b0100011: return decode_store(w);                 // RV32I: stores
            case 0b1100011: return decode_branch(w);                // RV32I: control transfer (branches)
            case 0b1101111: return decode_jumps(w, Op::JAL);        // RV32I: control transfer (jumps -- jal)
            case 0b1100111: return decode_jumps(w, Op::JALR);       // RV32I: control transfer (jumps -- jalr)
            case 0b0110111: return decode_upp_imm(w, Op::LUI);      // RV32I: upper immediate (lui)
            case 0b0010111: return decode_upp_imm(w, Op::AUIPC);    // RV32I: upper immediate (auipc)
            case 0b0001111: return decode_fence(w);                 // RV32I: fence (fence)
            case 0b1110011: return decode_system(w);                // RV32I: system (ecall/ebreak/mret/csr*)
        }
        spdlog::critical("Decoder: illegal/unrecognized instruction encoding 0x{:08x}", w);
        return illegal_instruction(w);
    }

private:
    using BitsW = pebble::utils::Bits<word_t>;
    static constexpr word_t opcode_mask_ = BitsW::mask(7);  // bits[0...6] are the opcode bits in every instruction

    static Instruction decode_reg_reg(word_t w) noexcept;
    static Instruction decode_reg_imm(word_t w) noexcept;
    static Instruction decode_load(word_t w) noexcept;
    static Instruction decode_store(word_t w) noexcept;
    static Instruction decode_branch(word_t w) noexcept;
    static Instruction decode_jumps(word_t w, Op op) noexcept;
    static Instruction decode_upp_imm(word_t w, Op op) noexcept;
    static Instruction decode_fence(word_t w) noexcept;
    static Instruction decode_system(word_t w) noexcept;

    static constexpr Instruction illegal_instruction(word_t w) { return Instruction{.raw=w, .op=Op::ILLEGAL, .op_fam=OpFamily::Illegal}; }

    static RegId reg_id(word_t w, std::size_t lo, std::size_t hi) {
        return RegId{ static_cast<uint8_t>(BitsW::get_bits(w, lo, hi)) };
    }
};

}  // namespace pebble::isa
