#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <eda/EventSimulator.h>

// ============================================================================
// test_event_simulator.cpp — Phase 4 tests.
//
// Links eda_event (which pulls in eda_oop transitively).
// ============================================================================

using namespace eda;

namespace {

bool expectedXor(bool a, bool b) {
    return (a || b) && !(a && b); // written differently from XorGate's `a != b`, deliberately
}

void buildXorCircuit(EventSimulator& sim) {
    sim.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
    sim.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
    sim.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
    sim.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");
}

} // namespace

TEST(EventSimulatorTest, XorCircuitSettlesToCorrectValue) {
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            EventSimulator sim;
            buildXorCircuit(sim);
            sim.scheduleInputChange(0, "A", a);
            sim.scheduleInputChange(0, "B", b);
            sim.run();

            EXPECT_EQ(sim.wireValue("OUT"), expectedXor(a, b))
                << "XOR(" << a << ", " << b << ")";
        }
    }
}

TEST(EventSimulatorTest, NoEvaluationsWhenInputDoesNotActuallyChange) {
    EventSimulator sim;
    buildXorCircuit(sim);

    sim.scheduleInputChange(0, "A", false);
    sim.scheduleInputChange(0, "B", false);
    sim.run(); // first settle — some number of evaluations happen here

    sim.scheduleInputChange(100, "B", false); // already false — no real change
    std::size_t evaluations = sim.run();

    EXPECT_EQ(evaluations, 0u);
}

TEST(EventSimulatorTest, WatcherFiresOnlyOnActualTransition) {
    EventSimulator sim;
    buildXorCircuit(sim);

    std::vector<bool> observedValues;
    sim.addWatcher("OUT", [&observedValues](bool value) { observedValues.push_back(value); });

    // A=false, B=true deliberately — expectedXor(false,true) is `true`,
    // which differs from OUT's pre-seeded default (`false`), guaranteeing
    // a real transition happens. (A=false,B=false would settle to `false`
    // too — the SAME as the default — so no transition would be observed
    // at all; that's a real, separate boolean-only limitation, not a bug:
    // see docs/phases/phase4.md, "What this phase doesn't handle.")
    sim.scheduleInputChange(0, "A", false);
    sim.scheduleInputChange(0, "B", true);
    sim.run();
    std::size_t firstSettleCount = observedValues.size();
    EXPECT_GE(firstSettleCount, 1u); // OUT must have transitioned from its false default at least once

    sim.scheduleInputChange(100, "A", false); // same values — no transition, watcher must not fire again
    sim.scheduleInputChange(100, "B", true);
    sim.run();

    EXPECT_EQ(observedValues.size(), firstSettleCount);
}

TEST(EventSimulatorTest, OnlyAffectedBranchEvaluates) {
    // Two completely independent AND gates, no shared wires — the clean
    // demonstration of this phase's whole point.
    EventSimulator sim;
    sim.addComponent(std::make_unique<AndGate>(), {"x1", "x2"}, "outA");
    sim.addComponent(std::make_unique<AndGate>(), {"y1", "y2"}, "outB");

    sim.scheduleInputChange(0, "x1", true);
    sim.scheduleInputChange(0, "x2", true);
    sim.scheduleInputChange(0, "y1", true);
    sim.scheduleInputChange(0, "y2", true);
    sim.run(); // settle both branches once
    ASSERT_TRUE(sim.wireValue("outA"));
    ASSERT_TRUE(sim.wireValue("outB"));

    // Flip only x1 — outB's branch shares no wires with x1, so exactly
    // ONE component (outA's AndGate) should be re-evaluated; outB's
    // AndGate must not run at all.
    sim.scheduleInputChange(100, "x1", false);
    std::size_t evaluations = sim.run();

    EXPECT_EQ(evaluations, 1u);
    EXPECT_FALSE(sim.wireValue("outA"));
    EXPECT_TRUE(sim.wireValue("outB")); // untouched, still true from before
}
