#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <stdexcept>
#include "isa/disassembler.hpp"
#include "isa/instruction.hpp"
#include "isa/op.hpp"
#include "isa/reg_id.hpp"
#include "utils/cast.hpp"

namespace pebble::isa::debug {

std::string Disassembler::disassemble(const Instruction &instr) {
    if(instr.is_illegal()) return "illegal";

    std::ostringstream oss{};
    oss << mnemonic(instr.op) << " ";

    switch(instr.op_fam) {
        case OpFamily::RegReg:
            oss << reg_name(instr.rd) << ", " << reg_name(instr.rs1) << ", " << reg_name(instr.rs2);
            break;

        case OpFamily::RegImm:
            oss << reg_name(instr.rd) << ", " << reg_name(instr.rs1) << ", " << instr.imm;
            break;

        case OpFamily::Load:
            oss << reg_name(instr.rd) << ", " << instr.imm << "(" << reg_name(instr.rs1) << ")";
            break;

        case OpFamily::Store:
            oss << reg_name(instr.rs2) << ", " << instr.imm << "(" << reg_name(instr.rs1) << ")";
            break;

        case OpFamily::Branch:
            oss << reg_name(instr.rs1) << ", " << reg_name(instr.rs2) << ", " << instr.imm;
            break;

        // risc-v base ISA only has JAL and JALR
        case OpFamily::Jump:
            if(instr.op == Op::JAL) oss << reg_name(instr.rd) << ", " << instr.imm;
            else if(instr.op == Op::JALR) oss << reg_name(instr.rd) << ", " << reg_name(instr.rs1) << ", " << instr.imm;
            else return "illegal";
            break;

        case OpFamily::UppImm:
            oss << reg_name(instr.rd) << ", " << (utils::Cast::u32(instr.imm) >> 12);
            break;

        case OpFamily::Csr:
            oss << reg_name(instr.rd) << ", " << instr.csr_addr << ", ";
            if(instr.rs1.has_value()) oss << reg_name(instr.rs1);
            else oss << instr.imm;
            break;

        // nothing to print for these
        case OpFamily::System:
        case OpFamily::Illegal:  // this case should be caught by instr.is_illegal(); putting it here for completeness
            break;
    }

    return oss.str();
}

}  // namespace pebble::isa::debug
