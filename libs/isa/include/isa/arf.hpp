#pragma once

#include <array>
#include <cstdint>
#include "isa/reg_id.hpp"

namespace pebble::isa {

using arf_t = uint32_t;

/* ArchRegisterFile -- the RV32I architectural integer register file (x0...x31) */
class ArchRegisterFile {
public:
    ArchRegisterFile() = default;

    [[nodiscard]] arf_t read(RegId id) const noexcept { return regs_[id.index()]; }
    void reset() noexcept { regs_.fill(0); }

    void write(RegId id, arf_t value) {
        if(id.index() == 0) return;  // x0 hardwired to zero; writes silently discarded
        regs_[id.index()] = value;
    }

private:
    std::array<arf_t, 32> regs_{};
};

}  // namespace pebble::isa
