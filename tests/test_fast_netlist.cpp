#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <eda/FastNetlist.h>

// ============================================================================
// test_fast_netlist.cpp — Phase 12 tests.
//
// Mirrors test_netlist.cpp's cases first (FastNetlist must behave
// identically to Netlist from the outside — that's the whole point of a
// benchmark comparison being meaningful), then adds a case specific to
// FastNetlist's caching: simulating, THEN adding another component,
// THEN simulating again. If the "topology changed, cache is stale"
// invalidation in addComponent() were missing or wrong, the second
// simulate() would reuse a now-incomplete evaluation order and the new
// component's output would never be computed.
// ============================================================================

using namespace eda;

namespace {

bool expectedXor(bool a, bool b) {
    return (a || b) && !(a && b);
}

} // namespace

TEST(FastNetlistTest, SingleAndGate) {
    FastNetlist netlist;
    netlist.addComponent(std::make_unique<AndGate>(), {"A", "B"}, "OUT");
    netlist.setInput("A", true);
    netlist.setInput("B", true);
    netlist.simulate();
    EXPECT_TRUE(netlist.wireValue("OUT"));
}

TEST(FastNetlistTest, UnknownWireDefaultsFalse) {
    FastNetlist netlist;
    EXPECT_FALSE(netlist.wireValue("nonexistent"));
}

TEST(FastNetlistTest, SimulatesXorRegardlessOfInsertionOrder) {
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            FastNetlist netlist;
            // Same worst-case insertion order as test_netlist.cpp's
            // equivalent case — OUT depends on n2/n3, which depend on
            // n1, none of which exist yet when OUT is added.
            netlist.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");
            netlist.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
            netlist.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
            netlist.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");

            netlist.setInput("A", a);
            netlist.setInput("B", b);
            netlist.simulate();

            EXPECT_EQ(netlist.wireValue("OUT"), expectedXor(a, b))
                << "XOR(" << a << ", " << b << ")";
        }
    }
}

TEST(FastNetlistTest, ThrowsOnFeedbackLoop) {
    FastNetlist netlist;
    netlist.addComponent(std::make_unique<NotGate>(), {"Y"}, "X");
    netlist.addComponent(std::make_unique<NotGate>(), {"X"}, "Y");

    EXPECT_THROW(netlist.simulate(), std::runtime_error);
}

TEST(FastNetlistTest, CachedEvaluationOrderInvalidatedByLaterAddComponent) {
    FastNetlist netlist;
    netlist.addComponent(std::make_unique<AndGate>(), {"A", "B"}, "MID");
    netlist.setInput("A", true);
    netlist.setInput("B", true);

    netlist.simulate(); // builds and caches the (1-entry) evaluation order
    EXPECT_TRUE(netlist.wireValue("MID"));

    // Topology changes AFTER the cache was built.
    netlist.addComponent(std::make_unique<NotGate>(), {"MID"}, "OUT");
    netlist.simulate(); // must rebuild, not reuse the stale 1-entry order

    EXPECT_FALSE(netlist.wireValue("OUT")); // NOT(true) == false
}

TEST(FastNetlistTest, RepeatedSimulateWithChangingInputsStaysCorrect) {
    FastNetlist netlist;
    netlist.addComponent(std::make_unique<XorGate>(), {"A", "B"}, "OUT");

    for (int i = 0; i < 5; ++i) {
        const bool a = (i % 2) == 0;
        const bool b = (i % 3) == 0;
        netlist.setInput("A", a);
        netlist.setInput("B", b);
        netlist.simulate(); // same cached evaluation order every iteration
        EXPECT_EQ(netlist.wireValue("OUT"), a != b) << "iteration " << i;
    }
}
