#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace pebble::stats {

/* RunningAverage -- incrementally tracks mean and variance/stddev using
 * WELFORD's online algorithm. This avoids the numerical instability of
 * accumulating sum and sum-of-squares (naively) separately, which can
 * blow up once sample counts and/or values get extremely large */
class RunningAverage {
public:
    explicit RunningAverage(std::string name): name_{std::move(name)} {}

    void add(double v) {
        count_++;
        const double delta = v - mean_;
        mean_ += delta / static_cast<double>(count_);
        const double delta2 = v - mean_;
        m2_ += delta * delta2;
    }

    double variance() const noexcept {
        return count_ > 0 ? m2_ / static_cast<double>(count_): 0.0;
    }

    double stddev() const noexcept { return std::sqrt(variance()); }

    uint64_t count() const noexcept { return count_; }
    double mean() const noexcept { return mean_; }
    std::string_view name() const noexcept { return name_; }

private:
    const std::string name_{};
    uint64_t count_{0};
    double mean_{0.0};
    double m2_{0.0};
};

}  // namespace pebble::stats