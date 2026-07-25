#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <unordered_map>

namespace pebble::primitives {

/* EventQueue -- schedules callbacks to fire after a fixed number of cycles in
 * the future, for delayed effects that don't fit a plain per-cycle
 * Clockable (e.g. "this cache fill completes in N cycles", "this DRAM
 * command's data is ready after tCL cycles")
 *
 * Owns its own cycle counter, advanced by advance_cycle(). This is
 * deliberately decoupled from ClockDomain's global-cycle counter so an
 * EventQueue can be used per-domain (e.g. one for DRAM-side delayed
 * completions ticking at DRAM frequency) rather than assuming there is
 * exactly one global notion of "cycle" in the whole simulator
 */
class EventQueue {
public:
    EventQueue() = default;

    void schedule(uint64_t delay_cycles, std::function<void()> callback) {
        if(delay_cycles == 0)
            throw std::invalid_argument{"EventQueue() delay_cycles must be > 0"};

        const uint64_t fire_cycle = current_cycle_ + delay_cycles;
        pending_[fire_cycle].push_back(callback);
    }

    /* Advances the queue's own cycle counter by one and fires (in insertion order) every callback scheduled
     * to fire on the new current cycle */
    void advance_cycle() {
        current_cycle_++;
        auto it = pending_.find(current_cycle_);
        if(it == pending_.end()) return;

        auto callbacks = std::move(it->second);
        pending_.erase(it);

        for(auto &callback: callbacks)
            callback();
    }

    uint64_t current_cycle() const noexcept { return current_cycle_; }
    bool empty() const noexcept { return pending_.empty(); }

    std::size_t pending_event_count() const noexcept {
        std::size_t count = 0;
        for(const auto &[_, callbacks]: pending_) count += callbacks.size();
        return count;
    }

private:
    using function_vector = std::vector<std::function<void()>>;
    uint64_t current_cycle_{0};
    std::unordered_map<uint64_t, function_vector> pending_{};
};

}  // namespace pebble::primitives