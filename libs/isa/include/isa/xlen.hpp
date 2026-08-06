#pragma once

namespace pebble::isa {

/* Xlen -- the ISA's integer register width.
 * For now, only supports 32-bit since this project targets RV32I */
enum class Xlen {
    Xlen32,
};

}  // namespace pebble::isa
