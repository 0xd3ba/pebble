#include <cstdint>
#include "isa/decoder.hpp"
#include "isa/instruction.hpp"
#include "utils/bits.hpp"

namespace pebble::isa {

Instruction Decoder::decode_reg_reg(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::RegReg;
    i.rd = reg_id(w, 7, 11);
    i.rs1 = reg_id(w, 15, 19);
    i.rs2 = reg_id(w, 20, 24);
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));
    i.funct7 = static_cast<uint8_t>(BitsW::get_bits(w, 25, 31));

    // all m-extension instructions share same funct7 (0x01)
    // funct3 is 3-bits ranging from 0x0 to 0x7
    if (i.funct7 == 0x01) {
        // M extension
        switch (i.funct3) {
            case 0x0: i.op = Op::MUL; break;
            case 0x1: i.op = Op::MULH; break;
            case 0x2: i.op = Op::MULHSU; break;
            case 0x3: i.op = Op::MULHU; break;
            case 0x4: i.op = Op::DIV; break;
            case 0x5: i.op = Op::DIVU; break;
            case 0x6: i.op = Op::REM; break;
            case 0x7: i.op = Op::REMU; break;
            default:
                return illegal_instruction(w);
        }
    }

    else if (i.funct7 == 0x00) {
        switch (i.funct3) {
            case 0x0: i.op = Op::ADD; break;
            case 0x1: i.op = Op::SLL; break;
            case 0x2: i.op = Op::SLT; break;
            case 0x3: i.op = Op::SLTU; break;
            case 0x4: i.op = Op::XOR; break;
            case 0x5: i.op = Op::SRL; break;
            case 0x6: i.op = Op::OR; break;
            case 0x7: i.op = Op::AND; break;
            default:
                return illegal_instruction(w);
        }
    }

    else if (i.funct7 == 0x20) {
        switch (i.funct3) {
            case 0x0: i.op = Op::SUB; break;
            case 0x5: i.op = Op::SRA; break;
            default: return illegal_instruction(w);
        }
    }

    else return illegal_instruction(w);

    return i;
}

Instruction Decoder::decode_reg_imm(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::RegImm;
    i.rd = reg_id(w, 7, 11);
    i.rs1 = reg_id(w, 15, 19);
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));
    i.funct7 = static_cast<uint8_t>(BitsW::get_bits(w, 25, 31));

    word_t imm = BitsW::sign_extend(BitsW::get_bits(w, 20, 31), 12);  // 12-bit immediate field for all except 0x1 and 0x5

    // funct3 is 3-bits ranging from 0x0 to 0x7
    switch(i.funct3) {
        case 0x0: i.op = Op::ADDI; i.imm = imm; break;
        case 0x2: i.op = Op::SLTI; i.imm = imm; break;
        case 0x3: i.op = Op::SLTIU; i.imm = imm; break;
        case 0x4: i.op = Op::XORI; i.imm = imm; break;
        case 0x6: i.op = Op::ORI; i.imm = imm; break;
        case 0x7: i.op = Op::ANDI; i.imm = imm; break;

        // special cases for 0x1 and 0x5
        case 0x1:
            if(i.funct7 != 0x00) return illegal_instruction(w);
            i.op = Op::SLLI;
            i.imm = BitsW::get_bits(w, 20, 24);
            break;

        case 0x5:
            i.imm = BitsW::get_bits(w, 20, 24);
            if(i.funct7 == 0x00) i.op = Op::SRLI;
            else if(i.funct7 == 0x20) i.op = Op::SRAI;
            else
                return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_load(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::Load;
    i.rd = reg_id(w, 7, 11);
    i.rs1 = reg_id(w, 15, 19);
    i.imm = BitsW::sign_extend(BitsW::get_bits(w, 20, 31), 12);  // 12-bit immediate
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));

    // funct3 is 3-bits, but in loads, valid values are only 0,1,2,4,5 (no 3,6,7)
    switch(i.funct3) {
        case 0x0: i.op = Op::LB; break;
        case 0x1: i.op = Op::LH; break;
        case 0x2: i.op = Op::LW; break;
        case 0x4: i.op = Op::LBU; break;
        case 0x5: i.op = Op::LHU; break;
        default:
            return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_store(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::Store;
    i.rs1 = reg_id(w, 15, 19);
    i.rs2 = reg_id(w, 20, 24);
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));

    // imm[0:4] stored in w[7:11] and imm[5:11] stored in w[25:31]
    word_t imm = (BitsW::get_bits(w, 25, 31) << 5) | BitsW::get_bits(w, 7, 11);
    imm = BitsW::sign_extend(imm, 12);
    i.imm = imm;

    // funct3 is 3-bits, but in stores, valid values are only 0,1,2 (no 3...7)
    switch(i.funct3) {
        case 0x0: i.op = Op::SB; break;
        case 0x1: i.op = Op::SH; break;
        case 0x2: i.op = Op::SW; break;
        default:
            return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_branch(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::Branch;
    i.rs1 = reg_id(w, 15, 19);
    i.rs2 = reg_id(w, 20, 24);
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));

    /* one of the most confusing layout for immediate values (@_@)
     * w[31]    = imm[12]
     * w[7]     = imm[11]
     * w[25:30] = imm[5:10]
     * w[8:11]  = imm[1:4]
     *
     * immediate field in branches are 12-bits + 1 (=13) as imm[0] is implicitly 0
     */
    word_t imm = (BitsW::get_bits(w, 31, 31) << 12) |
        (BitsW::get_bits(w, 7, 7) << 11) |
        (BitsW::get_bits(w, 25, 30) << 5) |
        (BitsW::get_bits(w, 8, 11) << 1);

    imm = BitsW::sign_extend(imm, 13);
    i.imm = imm;

    // funct3 is 3-bits, but in branches, valid values are only 0,1,4,5,6,7 (no 2,3)
    switch(i.funct3) {
        case 0x0: i.op = Op::BEQ; break;
        case 0x1: i.op = Op::BNE; break;
        case 0x4: i.op = Op::BLT; break;
        case 0x5: i.op = Op::BGE; break;
        case 0x6: i.op = Op::BLTU; break;
        case 0x7: i.op = Op::BGEU; break;
        default:
            return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_jumps(word_t w, Op op) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::Jump;
    i.rd = reg_id(w, 7, 11);
    i.op = op;

    word_t imm{};

    switch(op) {
        case Op::JAL:
            /* yet another confusing layout for immediate values (@_@)
             * w[31]    = imm[20]
             * w[12:19] = imm[12:19]
             * w[20]    = imm[11]
             * w[21:30] = imm[1:10]
             *
             * immediate field in JAL is 21-bits as imm[0] is implicitly 0
             */
            imm = (BitsW::get_bits(w, 31, 31) << 20) |
                (BitsW::get_bits(w, 12, 19) << 12) |
                (BitsW::get_bits(w, 20, 20) << 11) |
                (BitsW::get_bits(w, 21, 30) << 1);
            imm = BitsW::sign_extend(imm, 21);
            i.imm = imm;
            break;

        case Op::JALR:
            i.rs1 = reg_id(w, 15, 19);
            i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));
            i.imm = BitsW::sign_extend(BitsW::get_bits(w, 20, 31), 12);
            if(i.funct3 != 0) return illegal_instruction(w);
            break;

        default:
            return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_upp_imm(word_t w, Op op) noexcept {
    Instruction i{};
    i.raw = w;
    i.op = op;
    i.op_fam = OpFamily::UppImm;

    switch(op) {
        case Op::LUI:  // fall through
        case Op::AUIPC:
            i.rd = reg_id(w, 7, 11);
            i.imm = w & BitsW::range_mask(12, 31);
            // bits [12:31] already occupy their final positions. No additional sign extension required
            break;
        default:
            return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_fence(word_t w) noexcept {
    Instruction i{};
    i.raw = w;
    i.op_fam = OpFamily::System;
    i.rd = reg_id(w, 7, 11);
    i.rs1 = reg_id(w, 15, 19);
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));

    // note: FENCE instruction is recognized, but treated as no-op for simplicity (for now)
    switch(i.funct3) {
        case 0x0: i.op = Op::FENCE; break;
        default: return illegal_instruction(w);
    }

    return i;
}

Instruction Decoder::decode_system(word_t w) noexcept {
    Instruction i{};
    word_t imm = BitsW::get_bits(w, 20, 31);
    i.raw = w;
    i.funct3 = static_cast<uint8_t>(BitsW::get_bits(w, 12, 14));

    // ecall, ebreak and mret
    if(i.funct3 == 0x0) {
        i.op_fam = OpFamily::System;
        i.imm = imm;

        if(i.imm == 0x0) i.op = Op::ECALL;
        else if(i.imm == 0x1) i.op = Op::EBREAK;
        else if(i.imm == 0x302) i.op = Op::MRET;
        else return illegal_instruction(w);

        // ensure rs1 and rd are zeros
        if(BitsW::get_bits(w, 7, 11) || BitsW::get_bits(w, 15, 19))
            return illegal_instruction(w);

        return i;
    }

    // funct3 != 0 -> csrr* ops
    i.op_fam = OpFamily::Csr;
    i.rd = reg_id(w, 7, 11);
    i.csr_addr = static_cast<uint16_t>(imm);

    RegId rs1_id = reg_id(w, 15, 19);
    uint8_t rs1_imm = BitsW::get_bits(w, 15, 19); // *i variants: bits 15:19 hold a 5-bit zero-extended immediate (unsigned imm)

    switch(i.funct3) {
        // case 0x0 already covered above
        case 0x1: i.op = Op::CSRRW; i.rs1 = rs1_id; break;
        case 0x2: i.op = Op::CSRRS; i.rs1 = rs1_id; break;
        case 0x3: i.op = Op::CSRRC; i.rs1 = rs1_id; break;
        // no case 0x4
        case 0x5: i.op = Op::CSRRWI; i.imm = static_cast<int32_t>(rs1_imm); break;
        case 0x6: i.op = Op::CSRRSI; i.imm = static_cast<int32_t>(rs1_imm); break;
        case 0x7: i.op = Op::CSRRCI; i.imm = static_cast<int32_t>(rs1_imm); break;
        default: return illegal_instruction(w);
    }

    return i;
}

}  // namespace pebble::isa