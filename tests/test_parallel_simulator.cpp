#include <gtest/gtest.h>
#include <memory>
#include <eda/ParallelSimulator.h>

// ============================================================================
// test_parallel_simulator.cpp — Phase 8 tests, part 2.
// ============================================================================

using namespace eda;

namespace {

bool expectedXor(bool a, bool b) {
    return (a || b) && !(a && b); // written differently from XorGate's `a != b`, deliberately
}

} // namespace

TEST(ParallelSimulatorTest, XorCircuitCorrectForAllInputsFourThreads) {
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            ParallelSimulator sim(4);
            sim.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
            sim.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
            sim.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
            sim.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

            sim.setInput("A", a);
            sim.setInput("B", b);
            sim.simulate();

            EXPECT_EQ(sim.wireValue("OUT"), expectedXor(a, b)) << "XOR(" << a << ", " << b << ")";
        }
    }
}

TEST(ParallelSimulatorTest, WorksCorrectlyWithASingleThread) {
    ParallelSimulator sim(1);
    sim.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
    sim.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
    sim.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
    sim.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

    sim.setInput("A", true);
    sim.setInput("B", false);
    sim.simulate();

    EXPECT_EQ(sim.wireValue("OUT"), expectedXor(true, false));
}

TEST(ParallelSimulatorTest, IndependentBranchesBothCorrectWhenRunConcurrently) {
    ParallelSimulator sim(4);
    sim.addComponent(std::make_unique<AndGate>(), {"x1", "x2"}, "outA");
    sim.addComponent(std::make_unique<OrGate>(), {"y1", "y2"}, "outB");

    sim.setInput("x1", true);
    sim.setInput("x2", false);
    sim.setInput("y1", false);
    sim.setInput("y2", false);
    sim.simulate();

    EXPECT_FALSE(sim.wireValue("outA")); // AND(true, false) = false
    EXPECT_FALSE(sim.wireValue("outB")); // OR(false, false) = false
}

TEST(ParallelSimulatorTest, EvaluationCountReflectsOnlyTheMostRecentSimulateCall) {
    ParallelSimulator sim(4);
    sim.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
    sim.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
    sim.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
    sim.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

    sim.setInput("A", true);
    sim.setInput("B", true);

    sim.simulate();
    EXPECT_EQ(sim.evaluationCount(), 4u);

    sim.simulate(); // same inputs, called again
    EXPECT_EQ(sim.evaluationCount(), 4u); // NOT 8 — must reset each call
}

TEST(ParallelSimulatorTest, ThrowsOnFeedbackLoop) {
    ParallelSimulator sim(2);
    sim.addComponent(std::make_unique<NotGate>(), {"Y"}, "X");
    sim.addComponent(std::make_unique<NotGate>(), {"X"}, "Y");

    EXPECT_THROW(sim.simulate(), std::runtime_error);
}

TEST(ParallelSimulatorTest, ManyIndependentGatesAllCorrectUnderConcurrency) {
    // A stress test, not just a single small case: more concurrently-
    // evaluated entries gives a real race condition more opportunity to
    // surface than one or two gates would.
    ParallelSimulator sim(8);
    constexpr int kGateCount = 30;

    for (int i = 0; i < kGateCount; ++i) {
        std::string suffix = std::to_string(i);
        bool a = (i % 2 == 0);
        bool b = (i % 3 == 0);
        sim.addComponent(std::make_unique<XorGate>(), {"x" + suffix, "y" + suffix}, "out" + suffix);
        sim.setInput("x" + suffix, a);
        sim.setInput("y" + suffix, b);
    }

    sim.simulate();

    for (int i = 0; i < kGateCount; ++i) {
        std::string suffix = std::to_string(i);
        bool a = (i % 2 == 0);
        bool b = (i % 3 == 0);
        EXPECT_EQ(sim.wireValue("out" + suffix), expectedXor(a, b)) << "gate " << i;
    }
    EXPECT_EQ(sim.evaluationCount(), static_cast<std::size_t>(kGateCount));
}
