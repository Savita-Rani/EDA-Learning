#include <iostream>
#include <eda/LogicGate.h>

// ============================================================================
// main.cpp — console driver for Phase 6.
//
// Same AND/OR/NOT/NAND/NOR/XOR gates as Phases 1/2, but driven by ONE
// generic LogicGate<T> implementation, instantiated twice: once for
// `bool` (must match Phase 1/2 exactly — sanity check), once for
// `LogicValue` (the actual new capability: 0/1/X/Z).
// ============================================================================

using namespace eda;

void printBoolTruthTable(const LogicGate<bool>& gate) {
    std::cout << "\n" << gate.name() << " (bool) truth table:\n";
    std::cout << "  A  B  | OUT\n  -------------\n";
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            std::cout << "  " << a << "  " << b << "  |  " << gate.evaluate(a, b) << "\n";
        }
    }
}

void printLogicValueTruthTable(const LogicGate<LogicValue>& gate) {
    std::cout << "\n" << gate.name() << " (LogicValue) truth table — 0/1/X/Z:\n";
    std::cout << "  A  B  | OUT\n  -------------\n";
    const LogicValue values[] = {LogicValue::Zero, LogicValue::One, LogicValue::Unknown, LogicValue::HighZ};
    for (LogicValue a : values) {
        for (LogicValue b : values) {
            std::cout << "  " << a << "  " << b << "  |  " << gate.evaluate(a, b) << "\n";
        }
    }
}

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 6: Templates ===\n";
    std::cout << "ONE LogicGate<T> template, two instantiations: bool and LogicValue.\n";

    std::cout << "\n--- bool instantiation (must match Phases 1/2 exactly) ---\n";
    printBoolTruthTable(LogicGate<bool>(GateKind::AND));
    printBoolTruthTable(LogicGate<bool>(GateKind::XOR));

    std::cout << "\n--- LogicValue instantiation (the new capability) ---\n";
    printLogicValueTruthTable(LogicGate<LogicValue>(GateKind::AND));
    printLogicValueTruthTable(LogicGate<LogicValue>(GateKind::XOR));

    std::cout << "\nNotice: AND(0, X) = 0 (0 dominates, stays definite);\n"
                 "        AND(1, X) = X (genuinely unknown);\n"
                 "        this is exactly the distinction Phase 4's EventSimulator\n"
                 "        couldn't make — an unset wire defaulted to plain `false`,\n"
                 "        indistinguishable from a real computed 0. With LogicValue,\n"
                 "        'never set' can be Unknown instead, distinct from a real 0.\n";

    return 0;
}
