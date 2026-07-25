#pragma once

#include <bit>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace pebble::primitives {

class BitsetPoolError: public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

/* BitsetPool - fixed-size pool of indices [0, capacity), each either free or allocated */
class BitsetPool {
public:
    BitsetPool() = delete;
    explicit BitsetPool(std::size_t capacity): capacity_{capacity} {
        if(capacity_ == 0)
            throw std::invalid_argument{"BitsetPool capacity must be > 0"};
        words_.assign(1 + (capacity_ - 1)/kWordBits, 0);

        // Set each bit to 1, indicating the entry is free (any bit after `capacity` bits (if any) are 0)
        for(std::size_t i=0; i<capacity; i++) {
            words_[i / kWordBits] |= uint64_t{1} << (i % kWordBits);
        }
    }

    [[nodiscard]] std::optional<std::size_t> allocate() {
        for(std::size_t i=0; i<words_.size(); i++) {
            if(!words_[i]) continue;
            const auto bit = static_cast<std::size_t>(std::countr_zero(words_[i]));  // no. of bits from right that are 0
            const auto index = bit + i*kWordBits;
            words_[i] &= ~(std::uint64_t{1} << bit);  //unset the bit
            allocated_count_++;
            return index;
        }
        return std::nullopt;
    }

    void free(std::size_t index) {
        if(index >= capacity_)
            throw BitsetPoolError{"BitsetPool::free() called on index which is out of range"};
        const auto w = index / kWordBits;
        const auto b = index % kWordBits;
        if(words_[w] & (uint64_t{1} << b))
            throw BitsetPoolError{"BitsetPool::free() called on index which is not allocated (double-free or never allocated)"};

        words_[w] |= (uint64_t{1} << b);
        allocated_count_--;
    }

    bool is_allocated(std::size_t index) const {
        if(index >= capacity_)
            throw BitsetPoolError{"BitsetPool::is_allocated() called on index which is out of range"};
        const auto w = index / kWordBits;
        const auto b = index % kWordBits;
        return !(words_[w] & (uint64_t{1} << b));
    }

    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t allocated_count() const noexcept { return allocated_count_; }
    std::size_t free_count() const noexcept { return capacity_ - allocated_count_; }
    bool full() const noexcept { return allocated_count_ == capacity_; }
    bool empty() const noexcept { return allocated_count_ == 0; }

private:
    static constexpr std::size_t kWordBits = 64;

    std::size_t capacity_;
    std::size_t allocated_count_{0};
    std::vector<uint64_t> words_;  // In each word, bit[i] = 1 means free
};

}  // namespace pebble::primitives