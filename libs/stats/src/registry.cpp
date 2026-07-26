#include <nlohmann/json.hpp>
#include "stats/registry.hpp"

namespace pebble::stats {

nlohmann::json StatsRegistry::to_json() const {
    nlohmann::json j{};

    for (const auto &[name, counter] : counters_)
        j[name] = {{"type", "counter"}, {"value", counter->value()}};

    for (const auto &[name, hist] : histograms_) {
        nlohmann::json buckets = nlohmann::json::array();
        const auto &b = hist->buckets();
        for (std::size_t i=0; i<b.size(); i++)
            buckets.push_back({{"bucket", i}, {"count", b[i]}});

        j[name] = {
            {"type", "histogram"},
            {"total_samples", hist->total_samples()},
            {"sum", hist->sum()},
            {"buckets", buckets},
        };
    }

    for (const auto &[name, avg] : running_averages_) {
        j[name] = {
            {"type", "running_average"},
            {"count", avg->count()},
            {"mean", avg->mean()},
            {"variance", avg->variance()},
            {"stddev", avg->stddev()},
        };
    }

    return j;
}

}  // namespace pebble::stats