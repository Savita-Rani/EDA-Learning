#include <gtest/gtest.h>
#include <eda/OwningPtr.h>

// ============================================================================
// test_owning_ptr.cpp — Phase 7 tests, part 1.
//
// Deliberately self-contained — no dependency on eda_oop/Component, since
// OwningPtr<T> is fully generic. Uses a tiny local Tracked type with a
// static counter to make construction/destruction OBSERVABLE, which
// Component (Phase 2) has no way to expose.
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

struct Base {
    virtual ~Base() = default;
    virtual int tag() const { return 0; }
};
struct Derived : Base {
    int tag() const override { return 1; }
};

} // namespace

TEST(OwningPtrTest, ConstructAndAccess) {
    Tracked::liveCount = 0;
    OwningPtr<Tracked> p = makeOwning<Tracked>(42);
    EXPECT_TRUE(static_cast<bool>(p));
    EXPECT_EQ(p->value, 42);
    EXPECT_EQ((*p).value, 42);
    EXPECT_EQ(Tracked::liveCount, 1);
}

TEST(OwningPtrTest, DestructorRunsOnScopeExit) {
    Tracked::liveCount = 0;
    {
        OwningPtr<Tracked> p = makeOwning<Tracked>(1);
        EXPECT_EQ(Tracked::liveCount, 1);
    }
    EXPECT_EQ(Tracked::liveCount, 0);
}

TEST(OwningPtrTest, MoveConstructorTransfersOwnership) {
    Tracked::liveCount = 0;
    OwningPtr<Tracked> a = makeOwning<Tracked>(7);
    OwningPtr<Tracked> b = std::move(a);

    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_EQ(a.get(), nullptr);
    EXPECT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(b->value, 7);
    EXPECT_EQ(Tracked::liveCount, 1); // still exactly one object alive
}

TEST(OwningPtrTest, MoveAssignmentDestroysPreviousOwnedObject) {
    Tracked::liveCount = 0;
    OwningPtr<Tracked> a = makeOwning<Tracked>(1);
    OwningPtr<Tracked> b = makeOwning<Tracked>(2);
    EXPECT_EQ(Tracked::liveCount, 2);

    b = std::move(a); // b's Tracked(2) must be deleted before taking a's

    EXPECT_EQ(Tracked::liveCount, 1);
    EXPECT_EQ(b->value, 1);
    EXPECT_FALSE(static_cast<bool>(a));
}

TEST(OwningPtrTest, ResetReplacesOwnedObject) {
    Tracked::liveCount = 0;
    OwningPtr<Tracked> p = makeOwning<Tracked>(1);
    p.reset(new Tracked(2));

    EXPECT_EQ(Tracked::liveCount, 1); // old Tracked(1) deleted by reset()
    EXPECT_EQ(p->value, 2);
}

TEST(OwningPtrTest, ReleaseHandsOffOwnershipWithoutDestroying) {
    Tracked::liveCount = 0;
    OwningPtr<Tracked> p = makeOwning<Tracked>(1);

    Tracked* raw = p.release();
    EXPECT_FALSE(static_cast<bool>(p));
    EXPECT_EQ(Tracked::liveCount, 1); // NOT destroyed — caller owns it now

    delete raw; // caller's responsibility, same as std::unique_ptr::release()
    EXPECT_EQ(Tracked::liveCount, 0);
}

TEST(OwningPtrTest, ConvertingMoveConstructorFromDerivedToBase) {
    // The whole point of the templated converting constructor: this
    // must compile and work, same as std::unique_ptr<Base> p =
    // std::make_unique<Derived>();
    OwningPtr<Base> p = makeOwning<Derived>();
    EXPECT_EQ(p->tag(), 1);
}
