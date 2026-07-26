#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace pebble::stats {

/* Counter -- a monotonically-increasing named statistic, modeling a
 * hardware performance counter (retired instructions, cache misses, branch mispredictions, etc.) */
class Counter {
public:
    Counter() = delete;
    explicit Counter(std::string name): name_{std::move(name)} {}

    void increment(uint64_t by = 1) noexcept { value_ += by; }
    void reset() noexcept { value_ = 0; }

    uint64_t value() const noexcept { return value_; }
    std::string_view name() const noexcept { return name_; }

private:
    const std::string name_{};
    uint64_t value_{0};
};

}  // namespace pebble::stats