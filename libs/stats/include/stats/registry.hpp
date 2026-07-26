#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "stats/counter.hpp"
#include "stats/histogram.hpp"
#include "stats/running_avg.hpp"

namespace pebble::stats {

/* StatsRegistry — owns every named statistic in the simulator and produces one JSON dump at end-of-run */
class StatsRegistry {
public:
    StatsRegistry() = default;

    Counter& register_counter(const std::string &name) { return register_stat(counters_, name); }
    Histogram& register_histogram(const std::string &name) { return register_stat(histograms_, name); }
    RunningAverage& register_running_average(const std::string &name) { return register_stat(running_averages_, name); }

    nlohmann::json to_json() const;
    std::string dump_json(int indent = 4) const { return to_json().dump(indent); }
    std::size_t stats_count() const noexcept { return counters_.size() + histograms_.size() + running_averages_.size(); }

private:
    template<typename T>
    using stats_map = std::unordered_map<std::string, std::unique_ptr<T>>;

    stats_map<Counter> counters_;
    stats_map<Histogram> histograms_;
    stats_map<RunningAverage> running_averages_;

    void check_if_name_available(const std::string &name) {
        if(counters_.count(name) || histograms_.count(name) || running_averages_.count(name))
            throw std::invalid_argument{"StatsRegistry::register_*() received a duplicate name: " + name};
    }

    template<typename T>
    T& register_stat(stats_map<T> &map, const std::string &name) {
        check_if_name_available(name);
        auto stat = std::make_unique<T>(name);
        T& ref = *stat;
        map.emplace(name, std::move(stat));
        return ref;
    }
};

}  // namespace pebble::stats
