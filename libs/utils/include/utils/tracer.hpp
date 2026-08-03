#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace pebble::utils {

/* Tracer -- Fixed-length buffer storing latest N items */
template<typename T, std::size_t N>
class Tracer {
public:
    Tracer() = default;

    void push_back(const T item) noexcept {
        buffer_[index_] = std::move(item);
        index_ = (index_ + 1) % size();
        count_++;
    }

    /* Operates from oldest -> newest */
    template<typename Fn>
    void for_each(Fn &&fn) const {
        const std::size_t n = std::min(count_, size());
        std::size_t remaining = n;
        std::size_t i = (count_ < size())? 0: index_;

        while(remaining-- > 0) {
            fn(buffer_[i]);
            i = (i+1) % n;
        }
    }

    uint64_t total_writes() const noexcept { return count_; }
    std::size_t size() const noexcept { return buffer_.size(); }

private:
    std::array<T, N> buffer_{};
    std::size_t index_{0};
    uint64_t count_{0};  // total writes to the buffer
};

}  // namespace pebble::utils