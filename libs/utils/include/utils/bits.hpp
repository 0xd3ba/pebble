#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace pebble::utils {

/* Bits -- pure utility class for handling bit-manipulations */
template<std::integral T>
class Bits {
public:
    using UT = std::make_unsigned_t<T>;
    using ST = std::make_signed_t<T>;
    static constexpr std::size_t kMaxBits = std::numeric_limits<T>::digits;

    Bits() = delete;

    /* Returns a bitmask where least-significant `width` bits are set */
    static constexpr UT mask(std::size_t width) {
        if(width > kMaxBits) throw std::invalid_argument{"Bits::bitmask(): width exceeds type width"};
        if(width == 0) return UT{0};
        if(width == kMaxBits) return static_cast<UT>(~UT{0});
        return static_cast<UT>((UT{1} << width) - UT{1});
    }

    /* Returns a bitmask where only the bits in range [lo, hi] are set */
    static constexpr UT range_mask(std::size_t lo, std::size_t hi) {
        if (hi < lo || hi >= kMaxBits)
            throw std::invalid_argument{"Bits::range_mask(): invalid range"};
        return static_cast<UT>(mask(hi - lo + 1) << lo);

    }

    /* Returns the bits in the range [lo, hi] from `val` */
    static constexpr UT get_bits(T val, std::size_t lo, std::size_t hi) {
        if (hi < lo || hi >= kMaxBits)
            throw std::invalid_argument{"Bits::get_bits(): invalid range"};

        UT uval = static_cast<UT>(val);
        return static_cast<UT>((uval >> lo) & mask(hi - lo + 1));
    }

    /* Returns the bit at position `pos` */
    static constexpr UT get_bit(T val, std::size_t pos) {
        if (pos >= kMaxBits)
            throw std::invalid_argument{"Bits::get_bit(): position out of range"};

        return static_cast<UT>((static_cast<UT>(val) >> pos) & UT{1});
    }

    /* Sets the bits in the range [lo, hi] */
    static constexpr T set_bits(T val, std::size_t lo, std::size_t hi) {
        if (hi < lo || hi >= kMaxBits)
            throw std::invalid_argument{"Bits::set_bits(): invalid range"};
        return static_cast<T>(static_cast<UT>(val) | range_mask(lo, hi));
    }

    /* Sets the bit in the given position `pos` */
    static constexpr T set_bit(T val, std::size_t pos) {
        if (pos >= kMaxBits)
            throw std::invalid_argument{"Bits::set_bit(): position out of range"};
        return static_cast<T>(static_cast<UT>(val) | (UT{1} << pos));
    }

    /* Clears the bit at position `pos` */
    static constexpr T clear_bit(T val, std::size_t pos) {
        if (pos >= kMaxBits)
            throw std::invalid_argument{"Bits::clear_bit(): position out of range"};
        return static_cast<T>(static_cast<UT>(val) & ~(UT{1} << pos));
    }

    /* Sign extends `val` to the given `width` */
    static constexpr ST sign_extend(T val, std::size_t width) {
        if (width == 0 || width > kMaxBits)
            throw std::invalid_argument{"Bits::sign_extend(): invalid width"};

        UT uval = static_cast<UT>(val) & mask(width);
        if (width == kMaxBits) return static_cast<ST>(uval);
        UT m = UT{1} << (width - 1);
        return static_cast<ST>((uval ^ m) - m);
    }

    /* Returns the lowest `width` bits and clear everything above them */
    static constexpr UT zero_extend(T val, std::size_t width) {
        if (width == 0 || width > kMaxBits)
            throw std::invalid_argument{"Bits::zero_extend(): invalid width"};
        return static_cast<UT>(val) & mask(width);
    }
};

}  // namespace pebble::utils