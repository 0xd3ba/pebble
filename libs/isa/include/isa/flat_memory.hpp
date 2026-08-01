#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include "isa/mem_width.hpp"
#include "isa/trap.hpp"

namespace pebble::isa {

namespace flat_memory {
struct ReadResult {
    uint32_t value{0};
    Trap trap{};
};
}  // namespace flat_memory

/* FlatMemory -- the functional oracle's byte-addressable backing store.
 * Purely for FunctionalCpu; the timing model's real memory hierarchy
 * (caches/DRAM, built later) never uses this directly. It's just the
 * reference that committed memory state gets diffed against, for correctness
 *
 * Little-endian byte packing, per the RISC-V specification */
class FlatMemory {
public:
    FlatMemory() = delete;
    explicit FlatMemory(std::size_t size_bytes): bytes_(size_bytes, 0) {}

    FlatMemory(const FlatMemory &other) = delete;
    FlatMemory& operator=(const FlatMemory &other) = delete;

    [[nodiscard]] flat_memory::ReadResult read(addr_t addr, MemWidth w) {
        Trap t = check_illegal_access(addr, w, /*is_load=*/true);
        if(t.kind != TrapKind::None) return flat_memory::ReadResult{.value=0, .trap=t};

        std::size_t n = mem_width_bytes(w);
        uint32_t value = 0;

        for(std::size_t i=0; i<n; i++)
            value |= (static_cast<uint32_t>(bytes_[addr + i]) << (8 * i));

        return flat_memory::ReadResult{.value=value, .trap=Trap::none()};
    }

    [[nodiscard]] Trap write(addr_t addr, MemWidth w, std::uint32_t value) {
        Trap t = check_illegal_access(addr, w, /*is_load=*/false);
        if(t.kind != TrapKind::None) return t;

        std::size_t n = mem_width_bytes(w);
        for(std::size_t i=0; i<n; i++)
            bytes_[addr + i] = static_cast<uint8_t>((value >> (8 * i)) & 0xff);

        return Trap::none();
    }

    std::size_t size() const noexcept { return bytes_.size(); }
    void clear() noexcept { std::fill(bytes_.begin(), bytes_.end(), 0); }

private:
    std::vector<uint8_t> bytes_;

    /* Checks for illegal access, if the address is misaligned or falls outside the provisioned memory space */
    [[nodiscard]] Trap check_illegal_access(addr_t addr, MemWidth w, bool is_load = true) {
        std::size_t n = mem_width_bytes(w);
        if(n>1 && (addr % n) != 0) {
            return Trap{
                .kind = is_load? TrapKind::LoadAddressMisaligned: TrapKind::StoreAddressMisaligned,
                .faulting_addr = addr,
                .message = "misaligned access at address=" + std::to_string(addr),
            };
        }

        const uint64_t end = static_cast<uint64_t>(addr) + n;  // to prevent overflow bug
        if(end > bytes_.size()) {
            return Trap{
                .kind = is_load? TrapKind::LoadAccessFault: TrapKind::StoreAccessFault,
                .faulting_addr = addr,
                .message = "access outside provisioned memory space at address=" + std::to_string(addr),
            };
        }

        return Trap::none(); // access is aligned and within valid range
    }
};

}  // namespace pebble::isa