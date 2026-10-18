#include <gtest/gtest.h>
#include <eda/SharedOwningPtr.h>

// ============================================================================
// test_shared_owning_ptr.cpp — Phase 7 tests, part 2.
// ============================================================================

using namespace eda;

namespace {

struct Tracked {
    static int liveCount;
    int value;
    explicit Tracked(int v) : value(v) { ++liveCount; }
    ~Tracked() { --liveCount; }
};
int Tracked::liveCount = 0;

} // namespace

TEST(SharedOwningPtrTest, ConstructAndUseCount) {
    Tracked::liveCount = 0;
    SharedOwningPtr<Tracked> p = makeShared<Tracked>(1);
    EXPECT_EQ(p.useCount(), 1u);
    EXPECT_EQ(p->value, 1);
}

TEST(SharedOwningPtrTest, CopyIncrementsUseCountOnBoth) {
    Tracked::liveCount = 0;
    SharedOwningPtr<Tracked> a = makeShared<Tracked>(1);
    SharedOwningPtr<Tracked> b = a;

    EXPECT_EQ(a.useCount(), 2u);
    EXPECT_EQ(b.useCount(), 2u); // same control block, same count
    EXPECT_EQ(Tracked::liveCount, 1); // still only ONE Tracked object
}

TEST(SharedOwningPtrTest, ObjectSurvivesUntilLastCopyIsDestroyed) {
    Tracked::liveCount = 0;
    SharedOwningPtr<Tracked> a = makeShared<Tracked>(1);
    {
        SharedOwningPtr<Tracked> b = a;
        EXPECT_EQ(Tracked::liveCount, 1);
    } // b destroyed here — count drops to 1, object must NOT be deleted yet

    EXPECT_EQ(Tracked::liveCount, 1);
    EXPECT_EQ(a.useCount(), 1u);
}

TEST(SharedOwningPtrTest, ObjectDeletedWhenLastCopyGoes) {
    Tracked::liveCount = 0;
    {
        SharedOwningPtr<Tracked> a = makeShared<Tracked>(1);
        SharedOwningPtr<Tracked> b = a;
        EXPECT_EQ(Tracked::liveCount, 1);
    } // both a and b destroyed — count reaches 0, object deleted

    EXPECT_EQ(Tracked::liveCount, 0);
}

TEST(SharedOwningPtrTest, MoveDoesNotIncrementUseCount) {
    Tracked::liveCount = 0;
    SharedOwningPtr<Tracked> a = makeShared<Tracked>(1);
    EXPECT_EQ(a.useCount(), 1u);

    SharedOwningPtr<Tracked> b = std::move(a);

    EXPECT_EQ(b.useCount(), 1u); // transferred, not shared further
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_EQ(a.useCount(), 0u);
}

TEST(SharedOwningPtrTest, SelfAssignmentIsSafe) {
    Tracked::liveCount = 0;
    SharedOwningPtr<Tracked> a = makeShared<Tracked>(1);

    a = a; // copy-and-swap handles this without special-casing

    EXPECT_EQ(a.useCount(), 1u);
    EXPECT_EQ(a->value, 1);
    EXPECT_EQ(Tracked::liveCount, 1);
}
