#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include "isa/addr.hpp"
#include "isa/reg_id.hpp"
#include "primitives/index.hpp"
#include "primitives/ring_buffer.hpp"
#include "uarch/prf.hpp"

namespace pebble::uarch {

/* Compile time upper-bound on the number of ROB entries. Actual upper-bound enforced at runtime by the ROB class */
constexpr std::size_t kMaxRobEntries = 1024;
using RobId = primitives::Index<kMaxRobEntries>;

/* RobEntry -- An in-flight instruction's ordering and rename bookkeeping */
struct RobEntry {
    isa::addr_t pc{0};
    std::optional<isa::RegId> dest_arch_reg_id{};
    std::optional<PhysRegId> dest_phys_reg_id{};
    std::optional<PhysRegId> old_phys_reg_id{};
    bool completed{false};
    bool valid{true};
};

/* ReorderBuffer -- tracks in-flight instructions in program order and their completion status.
 * Responsible only for ordering and completion bookkeeping */
class ReorderBuffer {
public:
    ReorderBuffer() = delete;
    explicit ReorderBuffer(std::size_t max_entries): max_entries_{max_entries}, rob_{max_entries} {
        if(max_entries == 0 || max_entries > kMaxRobEntries)
            throw std::invalid_argument{"ReorderBuffer: max_entries must be > 0 and <= " + std::to_string(kMaxRobEntries)};
    }

     /* Allocates a new entry at the tail. Returns its RobId, or std::nullopt if the buffer is full */
    [[nodiscard]] std::optional<RobId> allocate(RobEntry entry) {
        auto id = rob_.push_back(std::move(entry));  // returns std::nullopt when unable to push due to buffer being full
        if(!id.has_value()) return std::nullopt;
        return RobId{static_cast<uint32_t>(*id)};
    }

    /* Marks the entry as completed; called by execution writeback. */
    void mark_completed(RobId id) {
        check_valid(id);
        rob_[id.index()].completed = true;
    }

    /* Retires the oldest entry, advancing program-order commit by one. Throws if the buffer is empty or the head
     * is not yet completed -- in-order commit is the entire point of a ROB, so this invariant is enforced here itself */
    [[nodiscard]] const RobEntry retire() {
        // not using "is_head_completed()" deliberately to know the exact cause of the logical error, when it happens
        if(empty()) throw std::logic_error{"ReorderBuffer: retire() called on empty buffer"};
        if(!head().completed) throw std::logic_error{"ReorderBuffer: retire() called when oldest instruction not complete"};

        // copy to return to the caller; rob_.head() returns a reference, so avoiding it. statements below modify head, so need a copy
        auto retiring_entry = rob_[rob_.front_index()];
        rob_[rob_.front_index()].valid = false;
        rob_.pop_front();
        return retiring_entry;
    }

    [[nodiscard]] const RobEntry& head() const noexcept { return rob_.front(); }
    [[nodiscard]] bool is_head_completed() const noexcept { return !empty() && head().completed; }

    [[nodiscard]] std::size_t size() const noexcept { return rob_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return max_entries_; }
    [[nodiscard]] bool full() const noexcept { return size() == max_entries_; }
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    [[nodiscard]] RobEntry& operator[](RobId id) {
        check_valid(id);
        return rob_[id.index()];
    }

    [[nodiscard]] const RobEntry& operator[](RobId id) const {
        check_valid(id);
        return rob_[id.index()];
    }

private:
    std::size_t max_entries_;
    primitives::RingBuffer<RobEntry, kMaxRobEntries, primitives::RingBufferPolicy::Reject> rob_{};

    void check_valid(RobId id) const {
        std::size_t i = id.index();
        if(i >= max_entries_) throw std::out_of_range{"ReorderBuffer: index out of bounds: " + std::to_string(i)};
        if(!rob_[i].valid) throw std::invalid_argument{"ReorderBuffer: index refers to a retired/invalid entry: " + std::to_string(i)};
    }
};

}  // namespace pebble::uarch