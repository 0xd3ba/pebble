#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace pebble::primitives {

/* Index -- a generic strong wrapper for anything that requires an index (ID) */
template<std::size_t N>
class Index {
public:
    Index() = delete;
    explicit Index(uint32_t index): index_{index} {
        if(index >= N) throw std::out_of_range{"Id index out of range: " + std::to_string(index)};
    }

    bool operator==(const Index &other) const noexcept = default;
    bool operator!=(const Index &other) const noexcept = default;
    friend std::ostream& operator<<(std::ostream &os, const Index &id) { return os << std::to_string(id.index_); }

    uint32_t index() const noexcept { return index_; }
    std::size_t size() const noexcept { return N; }

private:
    uint32_t index_;
};

}  // namespace pebble::primitives

/* Hash support so Index can be used as a key in a map/set without fancy workarounds */
namespace std {
template<std::size_t N>
struct hash<pebble::primitives::Index<N>> {
    std::size_t operator()(const pebble::primitives::Index<N> &id) const noexcept {
        return std::hash<uint32_t>{}(id.index());
    }
};
}  // namespace std