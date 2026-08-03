#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "isa/trap.hpp"
#include "utils/cast.hpp"

namespace pebble::isa {

/* CsrIndex -- Register indices for the CSR file */
class CsrIndex {
public:
    static constexpr uint16_t kCycleLo = 0xc00;   // cycle counter bits[0:31]
    static constexpr uint16_t kCycleHi = 0xc80;   // cycle counter bits[32:63]
    static constexpr uint16_t kInsRetLo = 0xc02;  // instructions retired counter bits[0:31]
    static constexpr uint16_t kInsRetHi = 0xc82;  // instructions retired counter bits[32:63]
};

/* CsrFile -- A stripped down version RISC-V control/status register state this simulator actually needs.
 * FunctionalCpu doesn't deliver traps as hardware would (no PC redirect to a handler for example).
 * A Trap is just returned as data to the caller */
class CsrFile {
public:
    CsrFile() = default;

    [[nodiscard]] uint32_t read(uint16_t id) {
        if(id >= regs_.size()) throw std::out_of_range{"CsrFile::read: got invalid index: " + std::to_string(id)};
        switch(id) {
            case CsrIndex::kCycleLo:  return read_cycle_low();
            case CsrIndex::kCycleHi:  return read_cycle_high();
            case CsrIndex::kInsRetLo: return read_instret_low();
            case CsrIndex::kInsRetHi: return read_instret_high();
            default: return regs_[id];
        }
    }

    void write(uint16_t id, uint32_t value) {
        if(id >= regs_.size()) throw std::out_of_range{"CsrFile::write: got invalid index: " + std::to_string(id)};
        // skip writes for cycle and instructions retired counters
        switch(id) {
            case CsrIndex::kCycleLo:
            case CsrIndex::kCycleHi:
            case CsrIndex::kInsRetLo:
            case CsrIndex::kInsRetHi:
                return;
            default: regs_[id] = value;
        }
    }

    /* Convenience methods to read/update cycle counter and */
    void increment_cycle(uint64_t by = 1) noexcept { cycle_ += by; }
    void increment_instret(uint64_t by = 1) noexcept { instret_ += by; }

    /* Reads higher/lower 32-bits of the cycle or instret registers */
    [[nodiscard]] uint32_t read_cycle_high() const noexcept { return utils::Cast::u32(cycle_ >> 32); }
    [[nodiscard]] uint32_t read_cycle_low() const noexcept { return utils::Cast::u32(cycle_ & 0xffffffffu); }
    [[nodiscard]] uint32_t read_instret_high() const noexcept { return utils::Cast::u32(instret_ >> 32); }
    [[nodiscard]] uint32_t read_instret_low() const noexcept { return utils::Cast::u32(instret_ & 0xffffffffu); }
    [[nodiscard]] uint64_t read_cycle() const noexcept { return cycle_; }
    [[nodiscard]] uint64_t read_instret() const noexcept { return instret_; }

    /* Records the cause of the most recently observed trap. Takes a TrapKind directly (not a raw mcause encoding)
     * as we're not reproducing the real mcause bit-layout since nothing will decode it back out.
     * This is not a hardware-faithful encoding. Deliberately designed it this way, for simplicity. */
    void set_mcause(TrapKind kind) noexcept { mcause_ = kind; }
    [[nodiscard]] TrapKind read_mcause() const noexcept { return mcause_; }

    void set_mepc(addr_t mepc) noexcept { mepc_ = mepc; }
    [[nodiscard]] addr_t read_mepc() const noexcept { return mepc_; }

    void reset() noexcept {
        regs_.fill(0);
        mepc_ = 0;
        mcause_ = TrapKind::None;
        cycle_ = 0;
        instret_ = 0;
    }

    std::size_t size() { return regs_.size(); }

private:
    std::array<uint32_t, 4096> regs_{};  // 12-bit register id -> 4096 control/status registers
    TrapKind mcause_{TrapKind::None};    // The kind of trap caused by mepc
    addr_t mepc_{0};                     // PC that caused the trap
    uint64_t cycle_{0};
    uint64_t instret_{0};
};

}  // namespace pebble::isa
