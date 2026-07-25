#pragma once

#include <cstdint>
#include <set>
#include <stdexcept>
#include "primitives/clockable.hpp"

namespace pebble::primitives {

/* ClockDomain -- groups Clockable modules that share a single frequency,
 * expressed as an integer divider against a global cycle counter (e.g.
 * divider=1 for the core clock, divider=3 for a DRAM clock running at
 * 1/3 the core frequency). A single top-level driver owns the global
 * cycle counter and calls maybe_tick(global_cycle) once per global
 * cycle on every domain; each domain decides internally whether this
 * global cycle corresponds to one of its own ticks.
 */
class ClockDomain {
public:
    ClockDomain() = delete;
    explicit ClockDomain(uint64_t divider): divider_{divider} {
        if(divider_ == 0)
            throw std::invalid_argument{"ClockDomain() divider must be > 0"};
    }

    void register_clockable(Clockable *clockable) {
        if(clockable == nullptr)
            throw std::invalid_argument{"ClockDomain::register_clockable(): received nullptr"};
        members_.insert(clockable);
    }

    /* Called once per every global clock cycle by the top-level driver.
     * Ticks every member iff the global cycle falls under this domain's frequency */
    void maybe_tick(uint64_t global_cycle) {
        if(global_cycle % divider_ != 0) return;
        for(Clockable *member: members_)
            member->tick();
    }

    void reset_all() {
        for(Clockable *member: members_)
            member->reset();
    }

    uint64_t divider() const noexcept { return divider_; }
    std::size_t members_count() const noexcept { return members_.size(); }

private:
    uint64_t divider_;
    std::set<Clockable *> members_;
};

}  // namespace pebble::primitives
