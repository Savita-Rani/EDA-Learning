#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <eda/Netlist.h>

// ============================================================================
// test_netlist.cpp — Phase 3 tests.
//
// Links eda_netlist (which pulls in eda_oop transitively). Entry/
// wireValues_ are private to Netlist, so these tests go through the
// public API only — exactly what a real caller would do.
//
// The insertion-order test below is the real point: if Netlist::simulate()
// evaluated components in the order they were ADDED rather than computing
// a proper topological order, this test would fail (n1 wouldn't exist yet
// when n2/n3 are evaluated). Passing with components added in the WORST
// possible order is direct evidence the topological sort is doing real
// work, not just getting lucky with a convenient insertion order.
// ============================================================================

using namespace eda;

namespace {

bool expectedXor(bool a, bool b) {
    return (a || b) && !(a && b); // written differently from XorGate's `a != b`, deliberately
}

} // namespace

TEST(NetlistTest, SingleAndGate) {
    Netlist netlist;
    netlist.addComponent(std::make_unique<AndGate>(), {"A", "B"}, "OUT");
    netlist.setInput("A", true);
    netlist.setInput("B", true);
    netlist.simulate();
    EXPECT_TRUE(netlist.wireValue("OUT"));
}

TEST(NetlistTest, UnknownWireDefaultsFalse) {
    Netlist netlist;
    EXPECT_FALSE(netlist.wireValue("nonexistent"));
}

TEST(NetlistTest, SimulatesXorRegardlessOfInsertionOrder) {
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            Netlist netlist;
            // Added in REVERSE dependency order on purpose: OUT depends on
            // n2/n3, which depend on n1 — none of which exist yet when OUT
            // is added. Evaluation-in-insertion-order would get this wrong.
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

TEST(NetlistTest, ThrowsOnFeedbackLoop) {
    Netlist netlist;
    // X depends on Y, Y depends on X — no valid evaluation order exists.
    netlist.addComponent(std::make_unique<NotGate>(), {"Y"}, "X");
    netlist.addComponent(std::make_unique<NotGate>(), {"X"}, "Y");

    EXPECT_THROW(netlist.simulate(), std::runtime_error);
}
