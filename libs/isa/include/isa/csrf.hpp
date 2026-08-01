#pragma once

#include <cstdint>
#include "isa/trap.hpp"

namespace pebble::isa {

/* CsrFile -- A stripped down version RISC-V control/status register state this simulator actually needs.
 * FunctionalCpu doesn't deliver traps as hardware would (no PC redirect to a handler for example).
 * A Trap is just returned as data to the caller */
class CsrFile {
public:
    CsrFile() = default;

    void increment_cycle(uint64_t by = 1) noexcept { cycle_ += by; }
    void increment_instret(uint64_t by = 1) noexcept { instret_ += by; }

    /* Reads higher/lower 32-bits of the cycle or instret registers */
    [[nodiscard]] uint32_t read_cycle_high() const noexcept { return static_cast<uint32_t>(cycle_ >> 32); }
    [[nodiscard]] uint32_t read_cycle_low() const noexcept { return static_cast<uint32_t>(cycle_ & 0xffffffffu); }
    [[nodiscard]] uint32_t read_instret_high() const noexcept { return static_cast<uint32_t>(instret_ >> 32); }
    [[nodiscard]] uint32_t read_instret_low() const noexcept { return static_cast<uint32_t>(instret_ & 0xffffffffu); }

    /* Records the cause of the most recently observed trap. Takes a TrapKind directly (not a raw mcause encoding)
     * as we're not reproducing the real mcause bit-layout since nothing will decode it back out.
     * This is not a hardware-faithful encoding. Deliberately designed it this way, for simplicity. */
    void set_mcause(TrapKind kind) noexcept { mcause_ = kind; }
    [[nodiscard]] TrapKind read_mcause() const noexcept { return mcause_; }

    void reset() noexcept {
        cycle_ = 0;
        instret_ = 0;
        mcause_ = TrapKind::None;
    }

private:
    uint64_t cycle_{0};    // cycles passed
    uint64_t instret_{0};  // instructions retired
    TrapKind mcause_{TrapKind::None};
};

}  // namespace pebble::isa
