#pragma once

#include <bit>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace pebble::stats {

/* Histogram -- power-of-2 (log2) bucketed distribution. Suited to
 * long-tailed latency-style data (for e.g. cache/DRAM miss latency etc.)
 * where fixed linear buckets either waste resolution on the common short tail
 * or need an impractically large bucket count to cover rare large values
 *
 * Bucket 0 holds exactly value 0 (log2(0) is undefined, so
 * it gets its own bucket). For value >= 1, bucket index = number of
 * bits needed to represent value: bucket 1 = {1}, bucket 2 = {2,3},
 * bucket 3 = {4...7}, bucket 4 = {8...15}, etc., i.e. bucket i (i>=1)
 * covers [2^(i-1), 2^i - 1].
 *
 * Buckets grow dynamically as larger values arrive -- there's no fixed
 * upper bound, so an unexpectedly large latency doesn't get silently clipped into some "everything else above X" bucket
 */
class Histogram {
public:
    Histogram() = delete;
    explicit Histogram(std::string name): name_{std::move(name)} {}

    void add(uint64_t v) {
        auto idx = bucket_index(v);
        if(idx >= buckets_.size()) buckets_.resize(idx+1, 0);
        buckets_[idx]++;
        total_samples_++;
        sum_ += v;
    }

    static std::size_t bucket_index(uint64_t v) {
        if(v == 0) return 0;
        auto bits = static_cast<std::size_t>(64 - std::countl_zero(v));  // no. of bits required to represent `v`
        // e.g: 5 = 0000...101 (61 zeros to left -> 64-61 = 3 bits required)
        return bits;
    }

    std::size_t bucket_count(std::size_t idx) const noexcept {
        return idx < buckets_.size() ? buckets_[idx]: 0;
    }

    std::string_view name() const noexcept { return name_; }
    uint64_t total_samples() const noexcept { return total_samples_; }
    uint64_t sum() const noexcept { return sum_; }
    const std::vector<uint64_t>& buckets() const noexcept { return buckets_; }

private:
    const std::string name_{};
    std::vector<uint64_t> buckets_{};
    uint64_t total_samples_{0};
    uint64_t sum_{0};
};

}  // namespace pebble::stats
