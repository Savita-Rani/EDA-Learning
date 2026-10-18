#include <iostream>
#include <memory>
#include <eda/Netlist.h>

// ============================================================================
// main.cpp — console driver for Phase 3.
//
// Builds a NETLIST, not a single gate: a classic 4-NAND decomposition of
// XOR (NAND being a "universal gate" — any boolean function can be built
// from NAND alone is a real, well-known EDA fact):
//
//     n1 = NAND(A, B)
//     n2 = NAND(A, n1)
//     n3 = NAND(B, n1)
//   OUT  = NAND(n2, n3)
//
// This is the concrete reason topological sort matters: n2 and n3 can't
// be evaluated before n1 exists, and OUT can't be evaluated before BOTH
// n2 and n3 exist. Netlist::simulate() figures that ordering out itself —
// this file adds the components in a convenient order, not a "safe" one
// (n1 is added first here, but it wouldn't matter if it weren't).
// ============================================================================

using namespace eda;

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 3: Netlist (STL) ===\n";
    std::cout << "4-NAND XOR circuit: n1=NAND(A,B) n2=NAND(A,n1) n3=NAND(B,n1) OUT=NAND(n2,n3)\n";
    std::cout << "\n  A  B  | OUT\n  -------------\n";

    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            Netlist netlist;
            netlist.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
            netlist.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
            netlist.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
            netlist.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

            netlist.setInput("A", a);
            netlist.setInput("B", b);
            netlist.simulate();

            std::cout << "  " << a << "  " << b << "  |  " << netlist.wireValue("OUT") << "\n";
        }
    }

    return 0;
}
