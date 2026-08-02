#pragma once

#include <cstdint>
#include <stdexcept>
#include "isa/op.hpp"

namespace pebble::isa {

/* MemWidth -- access width for memory reads/writes.
 * Purely a byte count in disguise */
enum class MemWidth {
    Byte,
    Half,
    Word,
};

/* Returns the width in bytes */
inline std::size_t mem_width_bytes(MemWidth w) {
    switch (w) {
        case MemWidth::Byte: return 1;
        case MemWidth::Half: return 2;
        case MemWidth::Word: return 4;
        default:
            throw std::domain_error{"received unsupported MemWidth"};
    }
}

/* Returns the width of the memory access op */
inline MemWidth width_of_mem_op(Op op) {
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
            throw std::invalid_argument{"width_of_mem_op: not a load/store op"};
    }
}

}  // namespace pebble::isa