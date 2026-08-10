#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include "isa/addr.hpp"
#include "primitives/index.hpp"
#include "primitives/ring_buffer.hpp"
#include "uarch/rob.hpp"

namespace pebble::uarch {

/* Compile time upper-bound on the number of LSQ entries. Actual upper-bound enforced at runtime by the LSQ class */
// todo: instead of constants being spread out, better define them in a centralized config lib header
constexpr std::size_t kMaxLoadQEntries = 1024;
constexpr std::size_t kMaxStoreQEntries = 1024;
using LoadQId = primitives::Index<kMaxLoadQEntries>;
using StoreQId = primitives::Index<kMaxStoreQEntries>;

/* LsqForwardPolicy -- What a load with a known address should do about older, address-unknown stores in the store queue:
 *     - Conservative: stall until every older store's address is known; if no match, go to memory; if match found, stall until that store commits
 *     - AddressMatch: forward on exact address match; stall only if an older store's address is still unknown; else go to memory
 *     - Speculative:  #TODO */
enum class LsqForwardPolicyType { Conservative, AddressMatch, /* Speculative, */ };
enum class LsqLoadResolutionType { Forward, GoToMemory, Stall, };

/* LsqLoadDecision -- Outcome of a forward operation irrespective of policy */
struct LsqLoadDecision {
    LsqLoadResolutionType type;
    std::optional<isa::word_t> value{};  // only set when type=Forward
};

/* LsqEntry -- An in-flight load/store instruction's ordering and minimal bookkeeping */
struct LsqEntry {
    std::uint64_t seq_id;
    RobId rob_id;
    std::optional<isa::addr_t> address;
    std::optional<isa::word_t> store_value;  // only for stores; loads should ignore this value
};

/* LoadStoreQueue -- Tracks in-flight load/store instructions in program order */
template<LsqForwardPolicyType Policy = LsqForwardPolicyType::Conservative>
class LoadStoreQueue {
public:
    LoadStoreQueue() = delete;
    LoadStoreQueue(std::size_t max_entries_load, std::size_t max_entries_store):
        load_q_{max_entries_load}, store_q_{max_entries_store}
    {}

    [[nodiscard]] std::optional<LoadQId> allocate_load(RobId rob_id) {
        auto id = load_q_.push_back(LsqEntry{.seq_id=next_seq(), .rob_id=rob_id});
        if(!id.has_value()) return std::nullopt;
        return LoadQId{*id};
    }

    [[nodiscard]] std::optional<StoreQId> allocate_store(RobId rob_id) {
        auto id = store_q_.push_back(LsqEntry{.seq_id=next_seq(), .rob_id=rob_id});
        if(!id.has_value()) return std::nullopt;
        return StoreQId{*id};
    }

    void set_load_address(LoadQId id, isa::addr_t address) { load_q_[id.index()].address = address; }
    void set_store_address(StoreQId id, isa::addr_t address) { store_q_[id.index()].address = address; }
    void set_store_value(StoreQId id, isa::word_t value) { store_q_[id.index()].store_value = value; }

    /* Checks all in-flight stores older than the given load and returns the appropriate */
    [[nodiscard]] LsqLoadDecision try_forward(LoadQId id) {
        if(load_q_.empty()) throw std::logic_error{"LoadStoreQueue: try_forward(...) called on empty load queue"};
        if(store_q_.empty()) return LsqLoadDecision{.type=LsqLoadResolutionType::GoToMemory};

        const LsqEntry &entry = load_q_[id.index()];

        if constexpr (Policy == LsqForwardPolicyType::Conservative) return policy_conservative(entry);
        else if constexpr (Policy == LsqForwardPolicyType::AddressMatch) return policy_address_match(entry);
        else throw std::domain_error{"LoadStoreQueue: try_forward(...) called with unsupported forward policy"};
    }

    /* Retires the oldest store in the queue and, returns the entry for the caller to commit the architectural state.
     * Doesn't check if the instruction is complete or not, as it is not this class's responsibility */
    [[nodiscard]] LsqEntry retire_store(StoreQId id) {
        if(store_q_.size() == 0) throw std::logic_error{"LoadStoreQueue: retire_store() called on empty store queue"};
        if(store_q_.front_index() != id.index())
            throw std::logic_error{"LoadStoreQueue: retire_store() called on non-oldest id: " + std::to_string(id.index())};
        LsqEntry entry = store_q_.front();
        store_q_.pop_front();
        return entry;
    }

    /* Retires the oldest load in the queue; doesn't check if the instruction is complete or not, as it is not this class's responsibility */
    void retire_load(LoadQId id) {
        if(load_q_.size() == 0) throw std::logic_error{"LoadStoreQueue: retire_load() called on empty load queue"};
        if(load_q_.front_index() != id.index())
            throw std::logic_error{"LoadStoreQueue: retire_load() called on non-oldest id: " + std::to_string(id.index())};
        load_q_.pop_front();
    }

    /* Discards all the entries whose ROB id is STRICTLY younger than the provided id */
    void squash_after(RobId id) {
        if(load_q_.size() > 0) invalidate_after(id, load_q_);
        if(store_q_.size() > 0) invalidate_after(id, store_q_);
    }

private:
    std::uint64_t seq_{0};  // monotonically increasing counter; used for comparing ordering across load/store queues

    template<std::size_t N>
    using Xq = primitives::RingBuffer<LsqEntry, N, primitives::RingBufferPolicy::Reject>;
    Xq<kMaxLoadQEntries> load_q_;
    Xq<kMaxStoreQEntries> store_q_;

    uint64_t next_seq() noexcept { return seq_++; }

    template<std::size_t N>
    void invalidate_after(RobId id, Xq<N> &q) {
        auto index = q.index_when([id](const LsqEntry &e){ return e.rob_id == id; });
        if(!index.has_value()) return;
        q.truncate_after(*index);
    }

    /* Conservative policy -- Stall only if there is no older store, whose address is unresolved or
     * is aliasing with the load's address; otherwise go to memory */
    LsqLoadDecision policy_conservative(const LsqEntry &lq_e) {
        bool unresolved_older_store = false;
        bool aliasing_older_store = false;

        store_q_.for_each([&](const LsqEntry &sq_e) {
            if(sq_e.seq_id >= lq_e.seq_id) return;  // not older than this load
            if(!sq_e.address.has_value()) { unresolved_older_store = true; return; }
            if(sq_e.address == lq_e.address) aliasing_older_store = true;
        });

        // when aliasing_older_store=true, correctness requires waiting for this store to commit to memory before the load reads memory
        if(aliasing_older_store || unresolved_older_store)
            return LsqLoadDecision{.type=LsqLoadResolutionType::Stall};

        return LsqLoadDecision{.type=LsqLoadResolutionType::GoToMemory};
    }

    /* Address Match policy -- Stall only when
     *     - no match && atleast one older store has an unresolved address
     *     - match && there is atleast one YOUNGER store (than the match) whose address is unresolved
     * otherwise go to memory */
    LsqLoadDecision policy_address_match(const LsqEntry &lq_e) {
        bool unresolved_older_store = false;
        std::optional<isa::word_t> match_val;
        std::optional<uint64_t> unresolved_seq_id{};
        std::optional<uint64_t> match_seq_id{};

        store_q_.for_each([&](const LsqEntry &sq_e) {
            if(sq_e.seq_id >= lq_e.seq_id) return;
            if(!sq_e.address.has_value()) {
                unresolved_older_store = true;
                unresolved_seq_id = sq_e.seq_id;
                return;
            }

            if(sq_e.address == lq_e.address) {
                match_val = sq_e.store_value;
                match_seq_id = sq_e.seq_id;
            }
        });

        if(match_val.has_value()) {
            /* Forwarding is only safe if no unresolved store sits between the match and the load -- such a store
             * could itself alias and override the value that we'd otherwise forward */
            if(unresolved_seq_id.has_value() && *unresolved_seq_id > *match_seq_id)
                return LsqLoadDecision{.type=LsqLoadResolutionType::Stall};
            return LsqLoadDecision{.type=LsqLoadResolutionType::Forward, .value=match_val};
        }

        else if(unresolved_older_store) return LsqLoadDecision{.type=LsqLoadResolutionType::Stall};
        else return LsqLoadDecision{.type=LsqLoadResolutionType::GoToMemory};
    }
};

}  // namespace pebble::uarch