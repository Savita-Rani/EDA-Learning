#include <gtest/gtest.h>
#include <eda/LogicGate.h>

// ============================================================================
// test_logic_gate.cpp — Phase 6 tests, part 2: LogicGate<T> itself,
// instantiated for both bool and LogicValue.
// ============================================================================

using namespace eda;

namespace {

bool expectedBool(GateKind kind, bool a, bool b) {
    switch (kind) {
        case GateKind::AND:  return a && b;
        case GateKind::OR:   return a || b;
        case GateKind::NOT:  return !a;
        case GateKind::NAND: return !(a && b);
        case GateKind::NOR:  return !(a || b);
        case GateKind::XOR:  return a != b;
    }
    return false;
}

const GateKind kAllKinds[] = {GateKind::AND, GateKind::OR, GateKind::NOT,
                               GateKind::NAND, GateKind::NOR, GateKind::XOR};

} // namespace

TEST(LogicGateTest, BoolInstantiationMatchesPhase1Phase2TruthTables) {
    // LogicGate<bool> must behave EXACTLY like Phase 1's Gate / Phase 2's
    // Component for every gate kind — same behavior, generalized
    // implementation. This is the backward-compatibility sanity check.
    for (GateKind kind : kAllKinds) {
        LogicGate<bool> gate(kind);
        for (bool a : {false, true}) {
            for (bool b : {false, true}) {
                EXPECT_EQ(gate.evaluate(a, b), expectedBool(kind, a, b))
                    << gate.name() << "(" << a << ", " << b << ")";
            }
        }
    }
}

TEST(LogicGateTest, AndZeroDominatesEvenOverUnknown) {
    LogicGate<LogicValue> gate(GateKind::AND);
    EXPECT_EQ(gate.evaluate(LogicValue::Zero, LogicValue::Unknown), LogicValue::Zero);
    EXPECT_EQ(gate.evaluate(LogicValue::Zero, LogicValue::HighZ), LogicValue::Zero);
    EXPECT_EQ(gate.evaluate(LogicValue::One, LogicValue::Unknown), LogicValue::Unknown);
}

TEST(LogicGateTest, OrOneDominatesEvenOverUnknown) {
    LogicGate<LogicValue> gate(GateKind::OR);
    EXPECT_EQ(gate.evaluate(LogicValue::One, LogicValue::Unknown), LogicValue::One);
    EXPECT_EQ(gate.evaluate(LogicValue::One, LogicValue::HighZ), LogicValue::One);
    EXPECT_EQ(gate.evaluate(LogicValue::Zero, LogicValue::Unknown), LogicValue::Unknown);
}

TEST(LogicGateTest, XorWithUnknownInputIsUnknown) {
    // XOR is DERIVED generically inside LogicGate<T>::evaluate() from
    // AND/OR/NOT — this test exercises that derivation for a type
    // LogicGate never has type-specific XOR code for.
    LogicGate<LogicValue> gate(GateKind::XOR);
    EXPECT_EQ(gate.evaluate(LogicValue::Unknown, LogicValue::Zero), LogicValue::Unknown);
    EXPECT_EQ(gate.evaluate(LogicValue::Unknown, LogicValue::One), LogicValue::Unknown);
}

TEST(LogicGateTest, HighZTreatedAsUnknownForLogicNotForDominance) {
    LogicGate<LogicValue> notGate(GateKind::NOT);
    EXPECT_EQ(notGate.evaluate(LogicValue::HighZ), LogicValue::Unknown);

    // HighZ does NOT dominate AND/OR the way Zero/One do — it only wins
    // when nothing else forces a definite result.
    LogicGate<LogicValue> andGate(GateKind::AND);
    EXPECT_EQ(andGate.evaluate(LogicValue::HighZ, LogicValue::Zero), LogicValue::Zero);
    EXPECT_EQ(andGate.evaluate(LogicValue::HighZ, LogicValue::One), LogicValue::Unknown);
}
