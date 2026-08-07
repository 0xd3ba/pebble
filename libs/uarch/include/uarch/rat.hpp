#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include "isa/reg_id.hpp"
#include "uarch/prf.hpp"

namespace pebble::uarch {
using areg_preg_map_t = std::unordered_map<isa::RegId, PhysRegId>;

/* RatSnapshot -- snapshot of the RegisterAliasTable */
struct RatSnapshot {
    const areg_preg_map_t snapshot{};
    bool operator==(const RatSnapshot &other) const = default;
    bool operator!=(const RatSnapshot &other) const = default;
};

/* RegisterAliasTable -- maps architectural register to a physical register */
class RegisterAliasTable {
public:
    RegisterAliasTable() = default;

    /* Maps arch_rid to phys_rid. Returns the mapping arch_rid had before this call
     * or std::nullopt if arch_rid had never been mapped yet */
    [[nodiscard]] std::optional<PhysRegId> remap(isa::RegId arch_rid, PhysRegId phys_rid) {
        auto it = map_.find(arch_rid);
        if(it == map_.end()) {
            // naive "map_[arch_rid] = phys_rid" will fail because default constructor of primitives::Index<N> is deleted
            map_.insert_or_assign(arch_rid, phys_rid);
            return std::nullopt;
        }

        PhysRegId old = it->second;
        it->second = phys_rid;
        return old;
    }

    [[nodiscard]] std::optional<PhysRegId> lookup(isa::RegId arch_rid) const noexcept {
        auto it = map_.find(arch_rid);
        if(it == map_.end()) return std::nullopt;
        return it->second;
    }

    [[nodiscard]] RatSnapshot checkpoint() const { return RatSnapshot{.snapshot=map_}; }
    void restore(RatSnapshot snapshot) { map_ = std::move(snapshot.snapshot); }

private:
    areg_preg_map_t map_{};
};

}  // namespace pebble::uarch