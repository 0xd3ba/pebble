#include <cstdint>
#include <stdexcept>
#include <gtest/gtest.h>
#include "utils/bits.hpp"

using namespace pebble::utils;

template <typename T>
class BitsTest : public ::testing::Test {};

using BitsTestTypes = ::testing::Types<uint8_t, uint16_t, uint32_t, uint64_t>;
TYPED_TEST_SUITE(BitsTest, BitsTestTypes);

TYPED_TEST(BitsTest, BitmaskZeroWidth) {
    using B = Bits<TypeParam>;
    EXPECT_EQ(B::mask(0), 0u);
}

TYPED_TEST(BitsTest, BitmaskFullWidth) {
    using B = Bits<TypeParam>;
    EXPECT_EQ(B::mask(B::kMaxBits), static_cast<typename B::UT>(~typename B::UT{0}));
}

TYPED_TEST(BitsTest, BitmaskPartialWidth) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 4) {
        EXPECT_EQ(B::mask(4), 0b1111u);
    }
    if (B::kMaxBits >= 1) {
        EXPECT_EQ(B::mask(1), 0b1u);
    }
}

TYPED_TEST(BitsTest, BitmaskThrowsOnExcessiveWidth) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::mask(B::kMaxBits + 1), std::invalid_argument);
}

TYPED_TEST(BitsTest, RangeMaskBasic) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        // bits [1:3] -> 0b0000_1110
        EXPECT_EQ(B::range_mask(1, 3), 0b1110u);
    }
}

TYPED_TEST(BitsTest, RangeMaskSingleBit) {
    using B = Bits<TypeParam>;
    EXPECT_EQ(B::range_mask(2, 2), 0b100u);
}

TYPED_TEST(BitsTest, RangeMaskFullWidth) {
    using B = Bits<TypeParam>;
    EXPECT_EQ(B::range_mask(0, B::kMaxBits-1), static_cast<typename B::UT>(~typename B::UT{0}));
}

TYPED_TEST(BitsTest, RangeMaskThrowsWhenHiLessThanLo) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::range_mask(3, 1), std::invalid_argument);
}

TYPED_TEST(BitsTest, RangeMaskThrowsWhenHiOutOfRange) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::range_mask(B::kMaxBits, 0), std::invalid_argument);
}

TYPED_TEST(BitsTest, GetBitsExtractsMiddleField) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        TypeParam val = static_cast<TypeParam>(0b1011'0110);
        // bits [4:6] of 1011_0110 -> 011
        EXPECT_EQ(B::get_bits(val, 4, 6), 0b011u);
    }
}

TYPED_TEST(BitsTest, GetBitsFullRange) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(~TypeParam{0});
    EXPECT_EQ(B::get_bits(val, 0, B::kMaxBits-1), static_cast<typename B::UT>(~typename B::UT{0}));
}

TYPED_TEST(BitsTest, GetBitsSingleBitRange) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(0b100);
    EXPECT_EQ(B::get_bits(val, 2, 2), 1u);
    EXPECT_EQ(B::get_bits(val, 1, 1), 0u);
}

TYPED_TEST(BitsTest, GetBitsThrowsOnInvalidRange) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::get_bits(TypeParam{0}, 3, 1), std::invalid_argument);
    EXPECT_THROW(B::get_bits(TypeParam{0}, 0, B::kMaxBits), std::invalid_argument);
}

TYPED_TEST(BitsTest, GetBitReturnsCorrectValue) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(0b1010);
    EXPECT_EQ(B::get_bit(val, 0), 0u);
    EXPECT_EQ(B::get_bit(val, 1), 1u);
    EXPECT_EQ(B::get_bit(val, 2), 0u);
    EXPECT_EQ(B::get_bit(val, 3), 1u);
}

TYPED_TEST(BitsTest, GetBitThrowsOnOutOfRangePosition) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::get_bit(TypeParam{0}, B::kMaxBits), std::invalid_argument);
}

TYPED_TEST(BitsTest, SetBitsReplacesFieldPreservingRest) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        TypeParam val = static_cast<TypeParam>(0b1111'0000);
        // Replace bits [0:2] with 0b0111
        TypeParam result = B::set_bits(val, 0, 2);
        EXPECT_EQ(result, static_cast<TypeParam>(0b1111'0111));
    }
}

TYPED_TEST(BitsTest, SetBitSetsOnlyTargetBit) {
    using B = Bits<TypeParam>;
    TypeParam val = 0;
    TypeParam result = B::set_bit(val, 3);
    EXPECT_EQ(result, static_cast<TypeParam>(0b1000));
}

TYPED_TEST(BitsTest, SetBitIsIdempotent) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(0b1000);
    TypeParam result = B::set_bit(val, 3);
    EXPECT_EQ(result, val);
}

TYPED_TEST(BitsTest, ClearBitClearsOnlyTargetBit) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(0b1111);
    TypeParam result = B::clear_bit(val, 1);
    EXPECT_EQ(result, static_cast<TypeParam>(0b1101));
}

TYPED_TEST(BitsTest, ClearBitIsIdempotent) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(0b1101);
    TypeParam result = B::clear_bit(val, 1);
    EXPECT_EQ(result, val);
}

TYPED_TEST(BitsTest, SetBitThrowsOnOutOfRangePosition) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::set_bit(TypeParam{0}, B::kMaxBits), std::invalid_argument);
}

TYPED_TEST(BitsTest, ClearBitThrowsOnOutOfRangePosition) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::clear_bit(TypeParam{0}, B::kMaxBits), std::invalid_argument);
}

TYPED_TEST(BitsTest, ZeroExtendMasksHigherBits) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        TypeParam val = static_cast<TypeParam>(0b1111'1010);
        EXPECT_EQ(B::zero_extend(val, 4), 0b1010u);
    }
}

TYPED_TEST(BitsTest, ZeroExtendFullWidthIsNoOp) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(~TypeParam{0});
    EXPECT_EQ(B::zero_extend(val, B::kMaxBits), static_cast<typename B::UT>(~typename B::UT{0}));
}

TYPED_TEST(BitsTest, ZeroExtendThrowsOnInvalidWidth) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::zero_extend(TypeParam{0}, 0), std::invalid_argument);
    EXPECT_THROW(B::zero_extend(TypeParam{0}, B::kMaxBits+1), std::invalid_argument);
}

TYPED_TEST(BitsTest, SignExtendPositiveValueStaysPositive) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        // 4-bit value 0b0011 (=3), sign bit (bit 3) is 0 -> stays 3
        TypeParam val = static_cast<TypeParam>(0b0011);
        EXPECT_EQ(B::sign_extend(val, 4), 3);
    }
}

TYPED_TEST(BitsTest, SignExtendNegativeValueBecomesNegative) {
    using B = Bits<TypeParam>;
    if (B::kMaxBits >= 8) {
        // 4-bit value 0b1010 (sign bit set) -> sign-extends to -6
        TypeParam val = static_cast<TypeParam>(0b1010);
        EXPECT_EQ(B::sign_extend(val, 4), -6);
    }
}

TYPED_TEST(BitsTest, SignExtendFullWidthReturnsSameBitPattern) {
    using B = Bits<TypeParam>;
    TypeParam val = static_cast<TypeParam>(~TypeParam{0}); // all bits set == -1
    EXPECT_EQ(B::sign_extend(val, B::kMaxBits), -1);
}

TYPED_TEST(BitsTest, SignExtendThrowsOnInvalidWidth) {
    using B = Bits<TypeParam>;
    EXPECT_THROW(B::sign_extend(TypeParam{0}, 0), std::invalid_argument);
    EXPECT_THROW(B::sign_extend(TypeParam{0}, B::kMaxBits+1), std::invalid_argument);
}

TEST(SignExtendFixedWidth, TwelveBitImmediateNegative) {
    using B = Bits<uint32_t>;
    // 12-bit immediate 0xFFF (all ones) == -1
    EXPECT_EQ(B::sign_extend(0xFFFu, 12), -1);
}

TEST(SignExtendFixedWidth, TwelveBitImmediatePositive) {
    using B = Bits<uint32_t>;
    // 12-bit immediate 0x7FF == max positive value = 2047
    EXPECT_EQ(B::sign_extend(0x7FFu, 12), 2047);
}

TEST(SignExtendFixedWidth, TwelveBitImmediateMinNegative) {
    using B = Bits<uint32_t>;
    // 12-bit immediate 0x800 (sign bit set, rest zero) == -2048
    EXPECT_EQ(B::sign_extend(0x800u, 12), -2048);
}

TEST(SignExtendFixedWidth, EightBitByteSignExtension) {
    using B = Bits<uint32_t>;
    EXPECT_EQ(B::sign_extend(0xFFu, 8), -1);   // 0xFF as int8_t == -1
    EXPECT_EQ(B::sign_extend(0x7Fu, 8), 127);
    EXPECT_EQ(B::sign_extend(0x80u, 8), -128);
}

TEST(SignedTypeInstantiation, GetBitsWorksOnSignedType) {
    using B = Bits<int32_t>;
    int32_t val = -1; // all bits set
    EXPECT_EQ(B::get_bits(val, 0, 3), 0b1111u);
}

TEST(SignedTypeInstantiation, SetBitOnSignedType) {
    using B = Bits<int32_t>;
    int32_t val = 0;
    int32_t result = B::set_bit(val, 0);
    EXPECT_EQ(result, 1);
}

TEST(SignedTypeInstantiation, SignExtendOnSignedType) {
    using B = Bits<int32_t>;
    int32_t val = 0b1010; // treat low 4 bits as a signed nibble
    EXPECT_EQ(B::sign_extend(val, 4), -6);
}
