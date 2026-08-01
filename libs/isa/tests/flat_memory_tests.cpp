#include <gtest/gtest.h>
#include "isa/flat_memory.hpp"
#include "isa/trap.hpp"

using namespace pebble::isa;

TEST(FlatMemoryTest, NewMemoryReadsAsZero) {
    FlatMemory mem{64};
    auto result = mem.read(0, MemWidth::Word);
    EXPECT_FALSE(result.trap.is_trap());
    EXPECT_EQ(result.value, 0);
}

TEST(FlatMemoryTest, WriteThenReadByteRoundTrips) {
    FlatMemory mem{64};
    Trap t = mem.write(0, MemWidth::Byte, 0xab);
    EXPECT_FALSE(t.is_trap());

    auto result = mem.read(0, MemWidth::Byte);
    EXPECT_FALSE(result.trap.is_trap());
    EXPECT_EQ(result.value, 0xab);
}

TEST(FlatMemoryTest, WriteThenReadHalfRoundTrips) {
    FlatMemory mem{64};
    auto trap = mem.write(0, MemWidth::Half, 0xd3ba);
    auto result = mem.read(0, MemWidth::Half);
    EXPECT_EQ(result.value, 0xd3ba);
}

TEST(FlatMemoryTest, WriteThenReadWordRoundTrips) {
    FlatMemory mem{64};
    auto trap = mem.write(0, MemWidth::Word, 0xd3bad3ba);
    auto result = mem.read(0, MemWidth::Word);
    EXPECT_EQ(result.value, 0xd3bad3ba);
}

TEST(FlatMemoryTest, LittleEndianByteOrdering) {
    // word 0x11223344 written at addr 0 should place 0x44 at byte 0, 0x33 at byte 1, 0x22 at byte 2, 0x11 at byte 3
    FlatMemory mem{64};
    auto trap = mem.write(0, MemWidth::Word, 0x11223344);
    EXPECT_EQ(mem.read(0, MemWidth::Byte).value, 0x44);
    EXPECT_EQ(mem.read(1, MemWidth::Byte).value, 0x33);
    EXPECT_EQ(mem.read(2, MemWidth::Byte).value, 0x22);
    EXPECT_EQ(mem.read(3, MemWidth::Byte).value, 0x11);
}

TEST(FlatMemoryTest, WriteTruncatesToWidth) {
    // Writing a value wider than the width should only store the low bytes. For e.g. writing 0x1122 as a Byte should only store 0x22
    FlatMemory mem{64};
    auto trap = mem.write(0, MemWidth::Byte, 0x1122);
    EXPECT_EQ(mem.read(0, MemWidth::Byte).value, 0x22);
}

TEST(FlatMemoryTest, ByteWriteDoesNotDisturbAdjacentBytes) {
    FlatMemory mem{64};
    auto trap1 = mem.write(0, MemWidth::Word, 0xffffffff);
    auto trap2 = mem.write(1, MemWidth::Byte, 0x00);
    EXPECT_EQ(mem.read(0, MemWidth::Byte).value, 0xff);
    EXPECT_EQ(mem.read(1, MemWidth::Byte).value, 0x00);
    EXPECT_EQ(mem.read(2, MemWidth::Byte).value, 0xff);
    EXPECT_EQ(mem.read(3, MemWidth::Byte).value, 0xff);
}

TEST(FlatMemoryTest, ByteAccessIsNeverMisaligned) {
    // Byte accesses have no alignment constraint at any address
    FlatMemory mem{64};
    auto result = mem.read(1, MemWidth::Byte);
    EXPECT_FALSE(result.trap.is_trap());
}

TEST(FlatMemoryTest, MisalignedHalfReadTrapsLoadAddressMisaligned) {
    FlatMemory mem{64};
    auto result = mem.read(1, MemWidth::Half);
    EXPECT_TRUE(result.trap.is_trap());
    EXPECT_EQ(result.trap.kind, TrapKind::LoadAddressMisaligned);
    ASSERT_TRUE(result.trap.faulting_addr.has_value());
    EXPECT_EQ(*result.trap.faulting_addr, 1);
}

TEST(FlatMemoryTest, MisalignedWordReadTrapsLoadAddressMisaligned) {
    FlatMemory mem{64};
    auto result = mem.read(2, MemWidth::Word);
    EXPECT_TRUE(result.trap.is_trap());
    EXPECT_EQ(result.trap.kind, TrapKind::LoadAddressMisaligned);
}

TEST(FlatMemoryTest, MisalignedHalfWriteTrapsStoreAddressMisaligned) {
    FlatMemory mem{64};
    Trap t = mem.write(1, MemWidth::Half, 0x1234);
    EXPECT_TRUE(t.is_trap());
    EXPECT_EQ(t.kind, TrapKind::StoreAddressMisaligned);
}

TEST(FlatMemoryTest, MisalignedWordWriteTrapsStoreAddressMisaligned) {
    FlatMemory mem{64};
    Trap t = mem.write(3, MemWidth::Word, 0x1234);
    EXPECT_TRUE(t.is_trap());
    EXPECT_EQ(t.kind, TrapKind::StoreAddressMisaligned);
}

TEST(FlatMemoryTest, AlignedHalfAtEvenAddressSucceeds) {
    FlatMemory mem{64};
    auto result = mem.read(4, MemWidth::Half);
    EXPECT_FALSE(result.trap.is_trap());
}

TEST(FlatMemoryTest, AlignedWordAtFourByteBoundarySucceeds) {
    FlatMemory mem{64};
    auto result = mem.read(8, MemWidth::Word);
    EXPECT_FALSE(result.trap.is_trap());
}

TEST(FlatMemoryTest, ReadPastEndTrapsLoadAccessFault) {
    FlatMemory mem{16};
    auto result = mem.read(16, MemWidth::Byte);  // one past last valid index (0...15)
    EXPECT_TRUE(result.trap.is_trap());
    EXPECT_EQ(result.trap.kind, TrapKind::LoadAccessFault);
    ASSERT_TRUE(result.trap.faulting_addr.has_value());
    EXPECT_EQ(*result.trap.faulting_addr, 16);
}

TEST(FlatMemoryTest, WordReadStraddlingEndOfMemoryTraps) {
    // 16-byte memory; word read at addr 13 would need bytes 13...16 but byte 16 is out of range.
    FlatMemory mem{16};
    auto result = mem.read(12, MemWidth::Word);  // in-bounds: 12...15, should succeed
    EXPECT_FALSE(result.trap.is_trap());
}

TEST(FlatMemoryTest, WritePastEndTrapsStoreAccessFault) {
    FlatMemory mem{16};
    Trap t = mem.write(16, MemWidth::Byte, 0xff);
    EXPECT_TRUE(t.is_trap());
    EXPECT_EQ(t.kind, TrapKind::StoreAccessFault);
}

TEST(FlatMemoryTest, LastValidByteIsAccessible) {
    FlatMemory mem{16};
    Trap t = mem.write(15, MemWidth::Byte, 0x42);
    EXPECT_FALSE(t.is_trap());
    EXPECT_EQ(mem.read(15, MemWidth::Byte).value, 0x42);
}

TEST(FlatMemoryTest, TrappedReadDoesNotCorruptReturnedValue) {
    /* A trapping read should yield value == 0, not garbage, so a caller that forgets to check the trap fails
     * predictably rather than silently consuming stale/undefined data */
    FlatMemory mem{4};
    auto result = mem.read(100, MemWidth::Byte);
    EXPECT_TRUE(result.trap.is_trap());
    EXPECT_EQ(result.value, 0);
}

TEST(FlatMemoryTest, FailedWriteDoesNotModifyMemory) {
    FlatMemory mem{4};
    auto trap = mem.write(0, MemWidth::Word, 0x11111111);
    Trap t = mem.write(100, MemWidth::Byte, 0xff);  // out of range, should fail
    EXPECT_TRUE(t.is_trap());
    EXPECT_EQ(mem.read(0, MemWidth::Word).value, 0x11111111);  // untouched
}

TEST(FlatMemoryTest, ClearResetsMemory) {
    FlatMemory mem{4};
    Trap t = mem.write(0, MemWidth::Word, 0x12345678);
    EXPECT_EQ(mem.read(0, MemWidth::Word).value, 0x12345678);
    mem.clear();
    EXPECT_EQ(mem.read(0, MemWidth::Word).value, 0);
}
