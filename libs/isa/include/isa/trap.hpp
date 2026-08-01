#pragma once

#include <optional>
#include <string>
#include "addr.hpp"

namespace pebble::isa {

/* TrapKind -- every architecturally-observable fault this simulator models.
 * Deliberately small: we're M-mode-only + no virtual memory, no OS; So
 * there's no page-fault/interrupt or any other faults to represent. Only the
 * faults that a bare-metal RV32IM core can actually raise against FlatMemory
 * and the decoder, plus the two System instructions we support */
enum class TrapKind {
    None,                      // no trap/fault occurred
    IllegalInstruction,        // unsupported/invalid instruction
    LoadAddressMisaligned,     // load address not aligned to word boundary
    StoreAddressMisaligned,    // store address not aligned to word boundary
    LoadAccessFault,           // load address outside FlatMemory's provisioned range
    StoreAccessFault,          // store address outside FlatMemory's provisioned range
    EnvironmentCallFromMMode,  // ecall
    Breakpoint,                // ebreak
};

/* Trap -- a record of an architecturally-observable fault,
 * carrying enough context (kind + faulting address + a human-readable note)
 * to actually debug a failing run rather than just knowing that something went wrong.
 * Note: faulting_addr is unused/nullopt for traps that aren't address-related (e.g. IllegalInstruction,
 * ecall, ebreak) */
struct Trap {
    TrapKind kind{TrapKind::None};
    std::optional<addr_t> faulting_addr{};
    std::string message{};

    [[nodiscard]] static Trap none() const { return Trap{}; }
    [[nodiscard]] bool is_trap() { return kind == TrapKind::None; }
};

}  // namespace pebble::isa