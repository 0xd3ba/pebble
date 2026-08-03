#pragma once

namespace pebble::isa {

/* OpFamily -- Which shape of operation an Op belongs to, independent of its specific semantics.*/
enum class OpFamily {
    RegReg,   // Register-register ALU
    RegImm,   // Register-immediate ALU
    Load,     // Memory load
    Store,    // Memory store
    Branch,   // Control branch
    Jump,     // Control jump
    UppImm,   // Upper immediate (LUI/AUIPC)
    System,   // FENCE/ECALL/EBREAK
    Csr,      // CSR read-modify-write
    Illegal,  // Illegal instruction
};

/* Op -- every RV32I + M instruction the decoder will recognize, plus Illegal for anything else
 * Reference: https://msyksphinz-self.github.io/riscv-isadoc/ */
enum class Op {

    /* RV32I: register-register ALU */
    ADD, SUB, XOR, OR, AND, SLL, SRL, SRA, SLT, SLTU,

    /* RV32I: register-immediate ALU */
    ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, SLTI, SLTIU,

    /* RV32I: loads/stores */
    LB, LH, LW, LBU, LHU, SB, SH, SW,

    /* RV32I: control transfer */
    BEQ, BNE, BLT, BGE, BLTU, BGEU, JAL, JALR,

    /* RV32I: upper immediate */
    LUI, AUIPC,

    /* RV32I: misc-memory / system / csr (modeled as recognized, functionally no-op/trap-triggering) -- can't ignore these */
    FENCE, ECALL, EBREAK, CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI,

    /* M extension */
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,

    /* Anything that doesn't match a legal RV32I+M encoding */
    ILLEGAL,
};

}  // namespace pebble::isa