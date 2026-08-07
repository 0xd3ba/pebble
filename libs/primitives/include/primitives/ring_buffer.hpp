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
    RingBuffer() = default;

    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] bool full() const noexcept { return count_ == N; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] static constexpr std::size_t capacity() { return N; }
    [[nodiscard]] const T& front() const noexcept { return buffer_[head_]; };
    [[nodiscard]] std::uint64_t total_writes() const noexcept { return total_writes_; }

    /* Allocates a slot at the tail, returning its index.
     * Policy::Reject:    returns nullopt if full
     * Policy::Overwrite: always succeeds, evicting the oldest entry if full */
    std::optional<std::size_t> push_back(T item) noexcept {
        if(full()) {
            if constexpr(Policy == RingBufferPolicy::Reject) return std::nullopt;
            // overwrite policy
            head_ = (head_ + 1) % N;
            count_--;  // will be incremented below
        }

        // append to the tail
        std::size_t tail = tail_;
        buffer_[tail] = std::move(item);

        tail_ = (tail_ + 1) % N;
        count_++;
        total_writes_++;

        return tail;
    }

    /* Removes the oldest entry; precondition: !empty() */
    void pop_front() {
        head_ = (head_ + 1) % N;
        if(count_ - 1 > count_) throw std::logic_error{"RingBuffer: pop_front() called on empty buffer"};
        count_--;
    }

    /* Goes through entries from oldest -> newest */
    template<typename Fn>
    void for_each(Fn &&fn) const {
        std::size_t i = head_;
        for(std::size_t n=0; n<count_; n++) {
            fn(buffer_[i]);
            i = (i + 1) % N;
        }
    }

    void reset() {
        buffer_.fill(0);
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    [[nodiscard]] const T& operator[](std::size_t i) const {
        if(i > N) throw std::out_of_range{"RingBuffer: index out of range " + std::to_string(i)};
        return buffer_[i];
    };

private:
    std::array<T, N> buffer_{};
    std::size_t head_{0};
    std::size_t tail_{0};
    std::uint64_t count_{0};         // total valid entries in the buffer; capped to N
    std::uint64_t total_writes_{0};  // total writes to the buffer (uncapped)
};

}  // namespace pebble::primitives