#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <eda/Component.h>

// ============================================================================
// test_component.cpp — Phase 2 tests.
//
// Separate file from test_gate.cpp (Phase 1), which is untouched. Same
// philosophy though: expected() independently recomputes each truth table
// instead of re-deriving it from the code under test, so a logic bug in
// AndGate::evaluate() etc. can't hide from these tests either.
//
// Links against eda_oop (Phase 2's library), not eda_core.
// ============================================================================

using namespace eda;

namespace {

bool expected(const char* gateName, bool a, bool b) {
    std::string name(gateName);
    if (name == "AND")  return a && b;
    if (name == "OR")   return a || b;
    if (name == "NOT")  return !a;
    if (name == "NAND") return !(a && b);
    if (name == "NOR")  return !(a || b);
    if (name == "XOR")  return (a || b) && !(a && b); // deliberately not `a != b`
    return false; // unreachable for the gates under test
}

void expectTruthTable(const Component& component) {
    const char* gateName = component.name();
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            EXPECT_EQ(component.evaluate(a, b), expected(gateName, a, b))
                << gateName << "(" << a << ", " << b << ")";
        }
    }
}

} // namespace

TEST(ComponentTest, AndGate)  { AndGate g;  EXPECT_STREQ(g.name(), "AND");  expectTruthTable(g); }
TEST(ComponentTest, OrGate)   { OrGate g;   EXPECT_STREQ(g.name(), "OR");   expectTruthTable(g); }
TEST(ComponentTest, NotGate)  { NotGate g;  EXPECT_STREQ(g.name(), "NOT");  expectTruthTable(g); }
TEST(ComponentTest, NandGate) { NandGate g; EXPECT_STREQ(g.name(), "NAND"); expectTruthTable(g); }
TEST(ComponentTest, NorGate)  { NorGate g;  EXPECT_STREQ(g.name(), "NOR");  expectTruthTable(g); }
TEST(ComponentTest, XorGate)  { XorGate g;  EXPECT_STREQ(g.name(), "XOR");  expectTruthTable(g); }

// Polymorphism-specific check: a single Component* can point at ANY derived
// gate, and evaluate()/name() dispatch to the right override at runtime.
// This is the behavior Phase 1's switch-based Gate has no equivalent of.
TEST(ComponentTest, PolymorphicDispatchThroughBasePointer) {
    std::vector<std::unique_ptr<Component>> gates;
    gates.push_back(std::make_unique<AndGate>());
    gates.push_back(std::make_unique<XorGate>());

    EXPECT_STREQ(gates[0]->name(), "AND");
    EXPECT_TRUE(gates[0]->evaluate(true, true));

    EXPECT_STREQ(gates[1]->name(), "XOR");
    EXPECT_TRUE(gates[1]->evaluate(true, false));
}
