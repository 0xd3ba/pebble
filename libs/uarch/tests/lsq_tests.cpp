#include <gtest/gtest.h>
#include "uarch/lsq.hpp"
#include "uarch/rob.hpp"

using namespace pebble::uarch;
using ConservativeLsq = LoadStoreQueue<LsqForwardPolicyType::Conservative>;
using AddressMatchLsq = LoadStoreQueue<LsqForwardPolicyType::AddressMatch>;

TEST(LoadStoreQueueTest, ConstructsWithGivenCapacities) {
    EXPECT_NO_THROW((ConservativeLsq{4, 4}));
}

TEST(LoadStoreQueueTest, AllocateLoadReturnsDistinctIds) {
    ConservativeLsq lsq{4, 4};

    auto a = lsq.allocate_load(RobId{0});
    auto b = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_NE(*a, *b);
}

TEST(LoadStoreQueueTest, LoadQueueFullDoesNotBlockStoreDispatch) {
    ConservativeLsq lsq{1, 4};

    ASSERT_TRUE(lsq.allocate_load(RobId{0}).has_value());
    ASSERT_EQ(lsq.allocate_load(RobId{1}), std::nullopt);  // load queue full

    // Store queue is independent capacity -- must still succeed.
    EXPECT_TRUE(lsq.allocate_store(RobId{2}).has_value());
}

TEST(LoadStoreQueueTest, StoreQueueFullDoesNotBlockLoadDispatch) {
    ConservativeLsq lsq{4, 1};
    ASSERT_TRUE(lsq.allocate_store(RobId{0}).has_value());
    ASSERT_EQ(lsq.allocate_store(RobId{1}), std::nullopt);  // store queue full

    EXPECT_TRUE(lsq.allocate_load(RobId{2}).has_value());
}

TEST(LoadStoreQueueTest, SquashAfterDiscardsYoungerEntriesOnly) {
    ConservativeLsq lsq{3, 3};
    auto l0 = lsq.allocate_load(RobId{0});
    auto l1 = lsq.allocate_load(RobId{1});
    auto l2 = lsq.allocate_load(RobId{2});
    ASSERT_TRUE(l0.has_value() && l1.has_value() && l2.has_value());

    lsq.squash_after(RobId{1});
    EXPECT_TRUE(lsq.allocate_load(RobId{3}).has_value());  // room for exactly 1 more
}

TEST(LoadStoreQueueTest, SquashAfterAffectsBothQueuesIndependently) {
    ConservativeLsq lsq{3, 3};
    lsq.allocate_load(RobId{0});
    lsq.allocate_store(RobId{1});
    lsq.allocate_load(RobId{2});
    lsq.allocate_store(RobId{3});

    lsq.squash_after(RobId{1});  // keeps load0, store1 and drops load2, store3

    EXPECT_TRUE(lsq.allocate_load(RobId{4}).has_value());
    EXPECT_TRUE(lsq.allocate_store(RobId{5}).has_value());
}

TEST(LoadStoreQueueTest, SquashAfterNonMemoryRobIdIsNoOp) {
    ConservativeLsq lsq{4, 4};
    lsq.allocate_load(RobId{0});
    lsq.allocate_store(RobId{1});

    // RobId{5} was never a load or store (not even present). squash_after(...) must not throw and must not discard anything
    EXPECT_NO_THROW(lsq.squash_after(RobId{5}));
}

TEST(LoadStoreQueueTest, SquashAfterOnEmptyQueuesIsNoOp) {
    ConservativeLsq lsq{4, 4};
    EXPECT_NO_THROW(lsq.squash_after(RobId{0}));
}

TEST(LoadStoreQueueTest, RetireStoreOnEmptyQueueThrows) {
    ConservativeLsq lsq{4, 4};
    EXPECT_THROW(lsq.retire_store(StoreQId{0}), std::logic_error);
}

TEST(LoadStoreQueueTest, RetireLoadOnEmptyQueueThrows) {
    ConservativeLsq lsq{4, 4};
    EXPECT_THROW(lsq.retire_load(LoadQId{0}), std::logic_error);
}

TEST(LoadStoreQueueTest, RetireStoreReturnsTheOldestEntry) {
    ConservativeLsq lsq{4, 4};
    auto id = lsq.allocate_store(RobId{7});

    ASSERT_TRUE(id.has_value());
    lsq.set_store_address(*id, 0x1000);
    lsq.set_store_value(*id, 123);

    LsqEntry retired = lsq.retire_store(*id);

    EXPECT_EQ(retired.rob_id, RobId{7});
    ASSERT_TRUE(retired.address.has_value());
    EXPECT_EQ(*retired.address, 0x1000);
    ASSERT_TRUE(retired.store_value.has_value());
    EXPECT_EQ(*retired.store_value, 123);
}

TEST(LoadStoreQueueTest, RetireStoreOnNonOldestIdThrows) {
    ConservativeLsq lsq{4, 4};
    auto s0 = lsq.allocate_store(RobId{0});
    auto s1 = lsq.allocate_store(RobId{1});

    ASSERT_TRUE(s0.has_value() && s1.has_value());
    EXPECT_THROW(lsq.retire_store(*s1), std::logic_error);
}

TEST(LoadStoreQueueTest, RetireLoadOnNonOldestIdThrows) {
    ConservativeLsq lsq{4, 4};
    auto l0 = lsq.allocate_load(RobId{0});
    auto l1 = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(l0.has_value() && l1.has_value());
    EXPECT_THROW(lsq.retire_load(*l1), std::logic_error);
}

TEST(LoadStoreQueueTest, RetireStoreFreesSlotForReallocation) {
    ConservativeLsq lsq{1, 1};
    auto id = lsq.allocate_store(RobId{0});

    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(lsq.allocate_store(RobId{1}), std::nullopt);  // full

    lsq.retire_store(*id);

    EXPECT_TRUE(lsq.allocate_store(RobId{2}).has_value());
}

TEST(LoadStoreQueueTest, RetireLoadFreesSlotForReallocation) {
    ConservativeLsq lsq{1, 1};
    auto id = lsq.allocate_load(RobId{0});
    ASSERT_TRUE(id.has_value());
    ASSERT_EQ(lsq.allocate_load(RobId{1}), std::nullopt);  // full

    lsq.retire_load(*id);

    EXPECT_TRUE(lsq.allocate_load(RobId{2}).has_value());
}

TEST(LoadStoreQueueTest, RetiresMustHappenInProgramOrder) {
    ConservativeLsq lsq{1, 2};
    auto s0 = lsq.allocate_store(RobId{0});
    auto s1 = lsq.allocate_store(RobId{1});
    ASSERT_TRUE(s0.has_value() && s1.has_value());

    // Retiring s1 first must fail even though nothing else about it is wrong
    EXPECT_THROW(lsq.retire_store(*s1), std::logic_error);

    // Correct order succeeds.
    EXPECT_NO_THROW(lsq.retire_store(*s0));
    EXPECT_NO_THROW(lsq.retire_store(*s1));
}

TEST(LoadStoreQueueTest, LoadAndStoreQueuesRetireIndependently) {
    ConservativeLsq lsq{1, 1};
    auto load = lsq.allocate_load(RobId{0});
    auto store = lsq.allocate_store(RobId{1});
    ASSERT_TRUE(load.has_value() && store.has_value());

    // Retiring the store must not require the load to retire first, or vice versa as they are independent queues.
    EXPECT_NO_THROW(lsq.retire_load(*load));
    EXPECT_NO_THROW(lsq.retire_store(*store));
}

TEST(LoadStoreQueueTest, TryForwardOnEmptyLoadQueueThrows) {
    ConservativeLsq lsq{4, 4};
    EXPECT_THROW(lsq.try_forward(LoadQId{0}), std::logic_error);
}

TEST(LoadStoreQueueTest, TryForwardWithEmptyStoreQueueGoesToMemory) {
    ConservativeLsq lsq{1, 1};
    auto load = lsq.allocate_load(RobId{0});

    ASSERT_TRUE(load.has_value());
    lsq.set_load_address(*load, 0x1000);

    auto decision = lsq.try_forward(*load);
    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}

TEST(LoadStoreQueueConservativeTest, NoOlderStoresGoesToMemory) {
    ConservativeLsq lsq{1, 1};
    auto store = lsq.allocate_store(RobId{0});
    auto load = lsq.allocate_load(RobId{1});  // load is younger than the store

    ASSERT_TRUE(store.has_value() && load.has_value());

    lsq.set_store_address(*store, 0x2000);
    lsq.set_load_address(*load, 0x1000);  // non-aliasing address
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}

TEST(LoadStoreQueueConservativeTest, YoungerStoreIgnoredByOlderLoad) {
    ConservativeLsq lsq{1, 1};
    auto load = lsq.allocate_load(RobId{0});
    auto store = lsq.allocate_store(RobId{1});  // store is younger than the load

    ASSERT_TRUE(load.has_value() && store.has_value());

    lsq.set_load_address(*load, 0x1000);
    lsq.set_store_address(*store, 0x1000);  // would alias, but is younger
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}

TEST(LoadStoreQueueConservativeTest, UnresolvedOlderStoreStalls) {
    ConservativeLsq lsq{1, 1};
    auto store = lsq.allocate_store(RobId{0});  // address not yet set; load should return stall on try_forward
    auto load = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(store.has_value() && load.has_value());

    lsq.set_load_address(*load, 0x1000);
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Stall);
}

TEST(LoadStoreQueueConservativeTest, AliasingOlderStoreStalls) {
    ConservativeLsq lsq{1, 1};
    auto store = lsq.allocate_store(RobId{0});
    auto load = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(store.has_value() && load.has_value());

    lsq.set_store_address(*store, 0x1000);
    lsq.set_store_value(*store, 42);
    lsq.set_load_address(*load, 0x1000);

    auto decision = lsq.try_forward(*load);

    // conservative policy should never forward a value directly, even on a match
    EXPECT_EQ(decision.type, LsqLoadResolutionType::Stall);
    EXPECT_EQ(decision.value, std::nullopt);
}

TEST(LoadStoreQueueConservativeTest, NonAliasingResolvedOlderStoresGoToMemory) {
    ConservativeLsq lsq{1, 2};
    auto s0 = lsq.allocate_store(RobId{0});
    auto s1 = lsq.allocate_store(RobId{1});
    auto load = lsq.allocate_load(RobId{2});

    ASSERT_TRUE(s0.has_value() && s1.has_value() && load.has_value());

    lsq.set_store_address(*s0, 0x1000);
    lsq.set_store_address(*s1, 0x2000);
    lsq.set_load_address(*load, 0x3000);

    auto decision = lsq.try_forward(*load);
    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}

TEST(LoadStoreQueueAddressMatchTest, MatchingResolvedOlderStoreForwardsValue) {
    AddressMatchLsq lsq{1, 1};
    auto store = lsq.allocate_store(RobId{0});
    auto load = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(store.has_value() && load.has_value());

    lsq.set_store_address(*store, 0x1000);
    lsq.set_store_value(*store, 99);
    lsq.set_load_address(*load, 0x1000);  // aliases with older store
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Forward);
    ASSERT_TRUE(decision.value.has_value());
    EXPECT_EQ(*decision.value, 99);
}

TEST(LoadStoreQueueAddressMatchTest, NoMatchNoUnresolvedGoesToMemory) {
    AddressMatchLsq lsq{1, 1};
    auto store = lsq.allocate_store(RobId{0});
    auto load = lsq.allocate_load(RobId{1});

    ASSERT_TRUE(store.has_value() && load.has_value());
    lsq.set_store_address(*store, 0x1000);
    lsq.set_load_address(*load, 0x2000);  // doesn't alias with older store
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}

TEST(LoadStoreQueueAddressMatchTest, UnresolvedOlderStoreWithNoMatchStalls) {
    AddressMatchLsq lsq{4, 4};
    auto store = lsq.allocate_store(RobId{0});
    auto load = lsq.allocate_load(RobId{1});
    ASSERT_TRUE(store.has_value() && load.has_value());

     // address unset for older store
    lsq.set_load_address(*load, 0x1000);
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Stall);
}

TEST(LoadStoreQueueAddressMatchTest, MatchFartherThanUnresolvedStoreStalls) {
    /* Order: match (oldest store) -> unresolved store -> load (youngest).
     * The unresolved store sits between the match and the load, so it
     * could itself alias and override the forwarded value -- must stall, not forward as per the policy */
    AddressMatchLsq lsq{2, 2};
    auto match = lsq.allocate_store(RobId{0});
    auto unresolved = lsq.allocate_store(RobId{1});
    auto load = lsq.allocate_load(RobId{2});

    ASSERT_TRUE(match.has_value() && unresolved.has_value() && load.has_value());

    lsq.set_store_address(*match, 0x1000);
    lsq.set_store_value(*match, 7);
    // unresolved's address intentionally left unset

    lsq.set_load_address(*load, 0x1000);
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Stall);
}

TEST(LoadStoreQueueAddressMatchTest, UnresolvedStoreOlderThanMatchStillForwards) {
    /* Order: unresolved store (oldest) -> match store -> load (youngest).
     * The unresolved store is farther from the load than the match, so
     * wherever it resolves it can't override what the match provides */
    AddressMatchLsq lsq{2, 2};
    auto unresolved = lsq.allocate_store(RobId{0});
    auto match = lsq.allocate_store(RobId{1});
    auto load = lsq.allocate_load(RobId{2});

    ASSERT_TRUE(unresolved.has_value() && match.has_value() && load.has_value());
    // unresolved's address intentionally left unset
    lsq.set_store_address(*match, 0x1000);
    lsq.set_store_value(*match, 55);
    lsq.set_load_address(*load, 0x1000);
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Forward);
    ASSERT_TRUE(decision.value.has_value());
    EXPECT_EQ(*decision.value, 55);
}

TEST(LoadStoreQueueAddressMatchTest, NearestMatchingStoreWinsOverOlderMatch) {
    /* Two aliasing stores, both resolved, both older than the load. The
     * load must see the nearer (younger) one's value, not the stale one */
    AddressMatchLsq lsq{2, 2};
    auto older_match = lsq.allocate_store(RobId{0});
    auto nearer_match = lsq.allocate_store(RobId{1});
    auto load = lsq.allocate_load(RobId{2});

    ASSERT_TRUE(older_match.has_value() && nearer_match.has_value() && load.has_value());

    lsq.set_store_address(*older_match, 0x1000);
    lsq.set_store_value(*older_match, 111);
    lsq.set_store_address(*nearer_match, 0x1000);
    lsq.set_store_value(*nearer_match, 222);
    lsq.set_load_address(*load, 0x1000);  // aliases with both older stores
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::Forward);
    ASSERT_TRUE(decision.value.has_value());
    EXPECT_EQ(*decision.value, 222);
}

TEST(LoadStoreQueueAddressMatchTest, YoungerStoreNeverConsidered) {
    AddressMatchLsq lsq{1, 1};
    auto load = lsq.allocate_load(RobId{0});
    auto younger_store = lsq.allocate_store(RobId{1});

    ASSERT_TRUE(load.has_value() && younger_store.has_value());
    lsq.set_load_address(*load, 0x1000);
    lsq.set_store_address(*younger_store, 0x1000); // aliases with the older load's address
    lsq.set_store_value(*younger_store, 999);
    auto decision = lsq.try_forward(*load);

    EXPECT_EQ(decision.type, LsqLoadResolutionType::GoToMemory);
}
