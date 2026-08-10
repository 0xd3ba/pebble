#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace pebble::primitives {

/* RingBufferPolicy -- What the buffer should do when it is full */
enum class RingBufferPolicy { Reject, Overwrite, };

/* RingBuffer -- Fixed-capacity ring buffer over a flat array.
 *     - Policy=Reject: push_back fails (returns std::nullopt) once full. Mainly for structures like a ROB where
 *       a full slot must never be silently lost
 *     - Policy=Overwrite: push_back always succeeds, evicting the oldest entry once full. Mainly for structures
 *       like a trace log where only the most recent N entries matter */
template<typename T, std::size_t N, RingBufferPolicy Policy = RingBufferPolicy::Reject>
class RingBuffer {
public:
    static_assert(N > 0, "RingBuffer: capacity must be > 0");

    RingBuffer(): max_entries_{N} {}
    explicit RingBuffer(std::size_t max_entries): max_entries_{max_entries} {
        if(max_entries == 0 || max_entries > N)
            throw std::invalid_argument{"RingBuffer: max_entries must be > 0 and <= " + std::to_string(N)};
    }

    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] bool full() const noexcept { return count_ == max_entries_; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t capacity() { return max_entries_; }
    [[nodiscard]] const T& front() const noexcept { return buffer_[head_]; };  // precondition: !empty()
    [[nodiscard]] std::size_t front_index() const noexcept { return head_; }
    [[nodiscard]] std::size_t back_index() const noexcept { return (tail_ + max_entries_ - 1) % max_entries_; }  // index to last valid element
    [[nodiscard]] std::uint64_t total_writes() const noexcept { return total_writes_; }

    /* Allocates a slot at the tail, returning its index.
     * Policy::Reject:    returns nullopt if full
     * Policy::Overwrite: always succeeds, evicting the oldest entry if full */
    std::optional<std::size_t> push_back(T item) noexcept {
        if(full()) {
            if constexpr(Policy == RingBufferPolicy::Reject) return std::nullopt;
            // overwrite policy
            head_ = (head_ + 1) % max_entries_;
            count_--;  // will be incremented below
        }

        // append to the tail
        std::size_t tail = tail_;
        buffer_[tail] = std::move(item);

        tail_ = (tail_ + 1) % max_entries_;
        count_++;
        total_writes_++;

        return tail;
    }

    /* Removes the oldest entry; precondition: !empty() */
    void pop_front() {
        if(count_ - 1 > count_) throw std::logic_error{"RingBuffer: pop_front() called on empty buffer"};
        head_ = (head_ + 1) % max_entries_;
        count_--;
    }

    void pop_back() {
        if(count_ - 1 > count_) throw std::logic_error{"RingBuffer: pop_back() called on empty buffer"};
        tail_ = back_index();
        count_--;
    }

    void truncate_after(std::optional<std::size_t> index) {
        while(!empty() && (!index.has_value() || back_index() != *index)) pop_back();
        if(empty() && index.has_value()) throw std::invalid_argument{"RingBuffer: truncate_after(...) called on invalid index"};
    }

    /* Goes through entries from oldest -> newest */
    template<typename Fn>
    void for_each(Fn &&fn) const {
        walk([&](std::size_t, const T &entry) {
            fn(entry);
            return false;  // don't stop early
        });
    }

    /* Returns first or last index to satisfy the condition when iterating from oldest -> newest; std::nullopt otherwise */
    template<typename Condition>
    std::optional<std::size_t> index_when(Condition &&fn, bool stop_on_found = true) {
        std::optional<std::size_t> found{};

        walk([&](std::size_t i, const T &entry) {
            if(!fn(entry)) return false;
            found = i;
            return stop_on_found;  // stop as soon as condition is satisfied if stop_on_found=true
        });

        return found;
    }

    void reset() {
        buffer_.fill(T{});
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    [[nodiscard]] const T& operator[](std::size_t i) const {
        if(i >= max_entries_) throw std::out_of_range{"RingBuffer: index out of range " + std::to_string(i)};
        return buffer_[i];
    };

    [[nodiscard]] T& operator[](std::size_t i) {
        if(i >= max_entries_) throw std::out_of_range{"RingBuffer: index out of range " + std::to_string(i)};
        return buffer_[i];
    };

private:
    std::array<T, N> buffer_{};
    const std::size_t max_entries_;
    std::size_t head_{0};
    std::size_t tail_{0};
    std::uint64_t count_{0};         // total valid entries in the buffer; capped to N
    std::uint64_t total_writes_{0};  // total writes to the buffer (uncapped)

    /* Visits live entries oldest -> newest, calling fn(index, entry) for each. Stops early if
     * fn returns true (treated as found/stop) */
    template <typename Fn>
    void walk(Fn&& fn) const {
        std::size_t i = head_;
        for (std::size_t n=0; n<count_; n++) {
            if (fn(i, buffer_[i])) return;
            i = (i + 1) % max_entries_;
        }
    }
};

}  // namespace pebble::primitives