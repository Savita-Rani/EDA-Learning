#include <gtest/gtest.h>
#include <string>
#include <eda/BufferGate.h>
#include <eda/ComponentVisitor.h>

using namespace eda;

namespace {

// A minimal Component subclass that exists ONLY in this test file —
// specifically so acceptVisitor() encounters a type it genuinely
// doesn't recognize, exercising the visitUnknown() fallback path for
// real rather than assuming it works.
class MysteryGate : public Component {
public:
    bool evaluate(bool a, bool /*b*/ = false) const override { return a; }
    const char* name() const override { return "MYSTERY"; }
};

struct RecordingVisitor : ComponentVisitor {
    std::string lastVisited;
    void visitAnd(const AndGate&) override { lastVisited = "AND"; }
    void visitOr(const OrGate&) override { lastVisited = "OR"; }
    void visitNot(const NotGate&) override { lastVisited = "NOT"; }
    void visitNand(const NandGate&) override { lastVisited = "NAND"; }
    void visitNor(const NorGate&) override { lastVisited = "NOR"; }
    void visitXor(const XorGate&) override { lastVisited = "XOR"; }
    void visitBuffer(const BufferGate&) override { lastVisited = "BUFFER"; }
    void visitUnknown(const Component&) override { lastVisited = "UNKNOWN"; }
};

} // namespace

TEST(ComponentVisitorTest, DispatchesEachKnownGateTypeCorrectly) {
    RecordingVisitor visitor;

    acceptVisitor(AndGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "AND");

    acceptVisitor(NandGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "NAND");

    acceptVisitor(XorGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "XOR");

    acceptVisitor(BufferGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "BUFFER");
}

TEST(ComponentVisitorTest, FallsBackToVisitUnknownForAnUnrecognizedType) {
    RecordingVisitor visitor;
    MysteryGate mystery;

    acceptVisitor(mystery, visitor);

    EXPECT_EQ(visitor.lastVisited, "UNKNOWN");
}

TEST(ComponentVisitorTest, DefaultVisitorMethodsAreNoOpsNotCrashes) {
    // A ComponentVisitor with every method left at its default (empty)
    // implementation must not crash when dispatched to.
    struct DoNothingVisitor : ComponentVisitor {};
    DoNothingVisitor visitor;

    acceptVisitor(AndGate(), visitor);
    acceptVisitor(BufferGate(), visitor);
    SUCCEED();
}
