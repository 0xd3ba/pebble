#pragma once

#include <cstdint>
#include <stdexcept>

namespace pebble::isa {

/* MemWidth -- access width for FlatMemory reads/writes. Purely a byte count in disguise */
enum class MemWidth {
    Byte,
    Half,
    Word,
};

/* Returns the width in bytes */
constexpr std::size_t mem_width_bytes(MemWidth w) {
    switch (w) {
        case MemWidth::Byte: return 1;
        case MemWidth::Half: return 2;
        case MemWidth::Word: return 4;
        default:
            throw std::domain_error{"received unsupported MemWidth"};
    }
}

}  // namespace pebble::isa