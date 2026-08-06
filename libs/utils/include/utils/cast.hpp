#pragma once

#include <cstdint>
#include <type_traits>

namespace pebble::utils {

/* Cast -- pure utility class for fixed-width integer conversions */
class Cast {
public:
    Cast() = delete;

    template<std::integral T> static constexpr int8_t i8(T v) noexcept { return static_cast<int8_t>(v); }
    template<std::integral T> static constexpr int16_t i16(T v) noexcept { return static_cast<int16_t>(v); }
    template<std::integral T> static constexpr int32_t i32(T v) noexcept { return static_cast<int32_t>(v); }
    template<std::integral T> static constexpr int64_t i64(T v) noexcept { return static_cast<int64_t>(v); }

    template<std::integral T> static constexpr uint8_t u8(T v) noexcept { return static_cast<uint8_t>(v); }
    template<std::integral T> static constexpr uint16_t u16(T v) noexcept { return static_cast<uint16_t>(v); }
    template<std::integral T> static constexpr uint32_t u32(T v) noexcept { return static_cast<uint32_t>(v); }
    template<std::integral T> static constexpr uint64_t u64(T v) noexcept { return static_cast<uint64_t>(v); }
};

}  // namespace pebble::utils