#include <string>
#include <gtest/gtest.h>
#include "primitives/register.hpp"

using namespace pebble::primitives;

TEST(RegisterTest, DefaultConstructedIsInvalid) {
    Register<int> r{};
    EXPECT_FALSE(r.valid());
}

TEST(RegisterTest, ReadOnDefaultConstructedThrows) {
    Register<int> r{};
    EXPECT_THROW(r.read(), InvalidRegisterRead);
}

TEST(RegisterTest, WriteThenReadRoundTrips) {
    Register<int> r{};
    r.write(42);
    EXPECT_TRUE(r.valid());
    EXPECT_EQ(r.read(), 42);
}

TEST(RegisterTest, WriteOverwritesPreviousValue) {
    Register<int> r{};
    r.write(1);
    r.write(2);
    EXPECT_EQ(r.read(), 2);
}

TEST(RegisterTest, InvalidateMakesInvalid) {
    Register<int> r{};
    r.write(7);
    r.invalidate();
    EXPECT_FALSE(r.valid());
    EXPECT_THROW(r.read(), InvalidRegisterRead);
}

TEST(RegisterTest, WriteAfterInvalidateIsValidAgain) {
    Register<int> r{};
    r.write(1);
    r.invalidate();
    r.write(2);
    EXPECT_TRUE(r.valid());
    EXPECT_EQ(r.read(), 2);
}

TEST(RegisterTest, ExplicitInitialValueConstructorStartsValid) {
    Register<int> r{5};
    EXPECT_TRUE(r.valid());
    EXPECT_EQ(r.read(), 5);
}

TEST(RegisterTest, WorksWithNonTrivialType) {
    Register<std::string> r{};
    EXPECT_FALSE(r.valid());
    r.write("hello");
    EXPECT_EQ(r.read(), "hello");
    r.invalidate();
    EXPECT_THROW(r.read(), InvalidRegisterRead);
}

TEST(RegisterTest, InvalidateDoesNotPreventSubsequentValidWrite) {
    Register<int> r{100};
    r.invalidate();
    r.write(200);
    EXPECT_EQ(r.read(), 200);
}
