#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pebble::primitives {

using CamHandle = std::size_t;

/* Cam<Key, Value> - models a content-addressable memory: parallel associative lookup by key
 * Duplicate keys are allowed ( for example, multiple in-flight stores to the same
 * address is a completely normal LSQ scenario) */
template<typename Key, typename Value>
class Cam {
public:
    struct Match {
        CamHandle handle;
        Value value;
    };

    Cam() = default;

    /* Inserts a new entry and returns its handle. Always succeeds (no fixed capacity) -- upto the caller to enforce this */
    CamHandle insert(Key key, Value value) {
        CamHandle handle = next_handle_++;
        entries_.emplace(handle, Entry{std::move(key), std::move(value)});
        return handle;
    }

    /* Associative search -- any entry that matches the `key` is returned */
    [[nodiscard]] std::vector<Match> lookup(const Key &key) const {
        std::vector<Match> matches;
        for(const auto &[handle, entry]: entries_) {
            if(entry.key != key) continue;
            matches.push_back(Match{handle, entry.value});
        }
        return matches;
    }

    /* Returns the first match encountered if there happen to be several.
     * Prefer lookup() if duplicates are possible and order/selection matters */
    [[nodiscard]] std::optional<Value> lookup_first(const Key &key) {
        for(const auto &[handle, entry]: entries_) {
            if(entry.key == key) return entry.value;
        }
        return std::nullopt;
    }

    bool remove(CamHandle handle) { return entries_.erase(handle) > 0; }
    bool contains(const CamHandle handle) const { return entries_.find(handle) != entries_.end(); }
    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }

private:
    struct Entry {
        Key key;
        Value value;
    };

    std::unordered_map<CamHandle, Entry> entries_{};
    CamHandle next_handle_{0};
};

}  // namespace pebble::primitives