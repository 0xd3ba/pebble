#pragma once

#include "primitives/index.hpp"

namespace pebble::isa {

/* RegId -- a validated RV32I integer register identifier for x0...x31.
 * Strong wrapper (not a naive uint8_t alias): constructing one from an out-of-range value is a decoder bug
 * (a 5-bit instruction field can never actually produce a value outside 0...31)
 *
 * Note: x0 is not special-cased here, RegId(0) is a perfectly valid RegId.
 * The "x0 always reads as zero, writes are discarded" rule is not enforced by the RegID */
using RegId = primitives::Index<32>;

}  // namespace pebble::isa