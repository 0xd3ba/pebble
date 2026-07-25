#include <string>
#include <gtest/gtest.h>
#include "primitives/latch.hpp"

using namespace pebble::primitives;

TEST(LatchTest, NewLatchHasNoOutput) {
    Latch<int> l{};
    EXPECT_FALSE(l.has_output());
}

TEST(LatchTest, GetOutputOnNewLatchThrows) {
    Latch<int> l{};
    EXPECT_THROW(l.get_output(), InvalidRegisterRead);
}

TEST(LatchTest, SetInputAloneDoesNotAffectOutput) {
    // Same-cycle read-after-write must be impossible: writing input this cycle must not be visible via get_output() until advance() fires
    Latch<int> l{};
    l.set_input(42);
    EXPECT_FALSE(l.has_output());
    EXPECT_THROW(l.get_output(), InvalidRegisterRead);
}

TEST(LatchTest, AdvanceMakesInputVisibleAsOutput) {
    Latch<int> l{};
    l.set_input(42);
    l.advance();
    EXPECT_TRUE(l.has_output());
    EXPECT_EQ(l.get_output(), 42);
}

TEST(LatchTest, AdvanceWithNoInputProducesBubble) {
    Latch<int> l{};
    l.set_input(1);
    l.advance();
    ASSERT_TRUE(l.has_output());

    l.advance();  // no set_input() this cycle
    EXPECT_FALSE(l.has_output());
    EXPECT_THROW(l.get_output(), InvalidRegisterRead);
}

TEST(LatchTest, DoesNotImplicitlyHoldStaleValueAcrossBubble) {
    /* Explicitly guards against "holds last value" semantics -- a stage
     * that wants to hold data across a stall must re-set_input() every
     * cycle; the latch must never silently repeat 1 here */
    Latch<int> l{};
    l.set_input(1);
    l.advance();
    l.advance();  // bubble
    l.set_input(2);
    l.advance();
    EXPECT_EQ(l.get_output(), 2);  // not "1 held over"
}

TEST(LatchTest, MultiCycleFlowThroughSequence) {
    Latch<int> l{};

    l.set_input(10);
    l.advance();
    EXPECT_EQ(l.get_output(), 10);

    l.set_input(20);
    l.advance();
    EXPECT_EQ(l.get_output(), 20);

    l.advance();  // bubble cycle
    EXPECT_FALSE(l.has_output());

    l.set_input(30);
    l.advance();
    EXPECT_EQ(l.get_output(), 30);
}

TEST(LatchTest, SquashClearsCurrentOutput) {
    Latch<int> l{};
    l.set_input(1);
    l.advance();
    ASSERT_TRUE(l.has_output());

    l.squash();
    EXPECT_FALSE(l.has_output());
    EXPECT_THROW(l.get_output(), InvalidRegisterRead);
}

TEST(LatchTest, SquashDiscardsPendingNotYetAdvancedInput) {
    Latch<int> l{};
    l.set_input(1);
    l.advance();  // 1 is now the visible output

    l.set_input(2);  // pending, not yet advanced
    l.squash();  // must kill both output (1) and pending input (2)

    EXPECT_FALSE(l.has_output());
    l.advance();  // no input was set after squash -> bubble, not 2
    EXPECT_FALSE(l.has_output());
}

TEST(LatchTest, SetInputAfterSquashWorksNormally) {
    Latch<int> l{};
    l.set_input(1);
    l.advance();
    l.squash();

    l.set_input(99);
    l.advance();
    EXPECT_TRUE(l.has_output());
    EXPECT_EQ(l.get_output(), 99);
}

TEST(LatchTest, SquashOnFreshLatchIsSafeNoOp) {
    Latch<int> l{};
    EXPECT_NO_THROW(l.squash());
    EXPECT_FALSE(l.has_output());
}

TEST(LatchTest, WorksWithNonTrivialType) {
    Latch<std::string> l;
    l.set_input("fetch_stage_output");
    l.advance();
    EXPECT_EQ(l.get_output(), "fetch_stage_output");
}
