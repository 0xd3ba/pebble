#pragma once

#include <cstdint>
#include <ostream>
#include <stdexcept>

namespace pebble::isa {

/* RegId -- a validated RV32I integer register identifier for x0...x31.
 * Strong wrapper (not a naive uint8_t alias): constructing one from an out-of-range value is a decoder bug
 * (a 5-bit instruction field can never actually produce a value outside 0...31)
 *
 * Note: x0 is not special-cased here, RegId(0) is a perfectly valid RegId.
 * The "x0 always reads as zero, writes are discarded" rule is not enforced by the RegID */
class RegId {
public:
    static constexpr uint8_t kMaxIndex = 31;

    RegId() = delete;
    explicit RegId(uint8_t index): index_{index} {
        if(index > kMaxIndex)
            throw std::out_of_range{"RegId index out of range (must be between [0, 31]): " + std::to_string(index)};
    }

    bool operator==(const RegId &other) const noexcept = default;
    bool operator!=(const RegId &other) const noexcept = default;
    friend std::ostream& operator<<(std::ostream &os, const RegId &r) { return os << "x" << static_cast<int>(r.index_); }

    uint8_t index() const noexcept { return index_; }

private:
    uint8_t index_;
};

}  // namespace pebble::isa

/* Hash support so RegId can be used as a key in a map/set without fancy workarounds */
namespace std {
template<>
struct hash<pebble::isa::RegId> {
    std::size_t operator()(const pebble::isa::RegId &r) const noexcept {
        return std::hash<uint8_t>{}(r.index());
    }
};
}  // namespace std