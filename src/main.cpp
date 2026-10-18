#include <iostream>
#include <vector>
#include "Gate.h"

// ============================================================================
// main.cpp — console driver for Phase 1.
//
// Concepts introduced here:
//  - #include <iostream>/<vector>  : standard library headers (angle
//    brackets) vs "Gate.h" (quotes) for our own project headers.
//  - std::vector<Gate>             : a dynamically-sized array template.
//    We build a list of every gate type once, then loop over it.
//  - range-based for loop          : `for (const auto& gate : gates)` walks
//    every element without manual indexing.
//  - std::cout                     : the standard output stream; `<<` is the
//    "insertion" operator that formats and appends.
//  - bool is printed as 1/0 by default; we convert manually for readability.
// ============================================================================

void printTruthTable(const Gate& gate) {
    std::cout << "\n" << gate.name() << " gate truth table:\n";
    std::cout << "  A  B  | OUT\n";
    std::cout << "  -------------\n";

    // Two-input truth table. For NOT, `b` is ignored by evaluate(), so the
    // repeated rows for b=false/true will show the same OUT — that's
    // expected and worth noticing.
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            bool out = gate.evaluate(a, b);
            std::cout << "  " << a << "  " << b << "  |  " << out << "\n";
        }
    }
}

int main() {
    std::vector<Gate> gates{
        Gate(GateType::AND),
        Gate(GateType::OR),
        Gate(GateType::NOT),
        Gate(GateType::NAND),
        Gate(GateType::NOR),
        Gate(GateType::XOR),
    };

    std::cout << "=== EDA Logic Simulator — Phase 1: Gate basics ===\n";

    for (const auto& gate : gates) {
        printTruthTable(gate);
    }

    return 0;
}
