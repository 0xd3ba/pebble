#pragma once

namespace pebble::isa {

/* PrivilegeLevel -- RISC-V privilege levels/modes.
 * For now, only supports the Machine level */
enum class PrivilegeLevel {
    Machine,
};

}  // namespace pebble::isa
