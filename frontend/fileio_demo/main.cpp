#include <iostream>
#include <eda/NetlistLoader.h>

// ============================================================================
// main.cpp — console driver for Phase 5.
//
// Same 4-NAND XOR circuit as Phases 3/4's demos — but this time it comes
// from circuits/xor4nand.nl, not addComponent() calls in this file.
// CIRCUIT_DIR is injected by CMakeLists.txt as an absolute path to that
// directory, so this works regardless of the current working directory
// the executable happens to be run from.
// ============================================================================

using namespace eda;

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 5: File I/O ===\n\n";

    const std::string goodPath = std::string(CIRCUIT_DIR) + "/xor4nand.nl";
    std::cout << "Loading: " << goodPath << "\n";

    try {
        Netlist netlist = loadNetlistFile(goodPath);

        std::cout << "  A  B  | OUT\n  -------------\n";
        for (bool a : {false, true}) {
            for (bool b : {false, true}) {
                netlist.setInput("A", a);
                netlist.setInput("B", b);
                netlist.simulate();
                std::cout << "  " << a << "  " << b << "  |  " << netlist.wireValue("OUT") << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load/simulate netlist: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\n-- Now loading a deliberately broken netlist file --\n";
    const std::string badPath = std::string(CIRCUIT_DIR) + "/broken_unknown_gate.nl";
    std::cout << "Loading: " << badPath << "\n";
    try {
        loadNetlistFile(badPath);
        std::cout << "(unexpectedly succeeded — this should not happen)\n";
    } catch (const std::exception& e) {
        std::cout << "Caught expected error: " << e.what() << "\n";
    }

    return 0;
}
