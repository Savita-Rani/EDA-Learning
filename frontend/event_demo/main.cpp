#include <iostream>
#include <memory>
#include <eda/EventSimulator.h>

// ============================================================================
// main.cpp — console driver for Phase 4.
//
// Same 4-NAND XOR circuit as Phase 3's netlist_demo, but driven by
// EventSimulator instead of Netlist — watch the evaluation counts to see
// the actual payoff: Phase 3's Netlist::simulate() always evaluates all 4
// gates, every call. EventSimulator only evaluates what a changed input
// actually affects — including "0 evaluations" when nothing changed.
// ============================================================================

using namespace eda;

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 4: Event-driven engine ===\n";
    std::cout << "Same 4-NAND XOR circuit as Phase 3, driven by events instead of a full recompute.\n\n";

    EventSimulator sim;
    sim.addComponent(std::make_unique<NandGate>(), {"A", "B"}, "n1");
    sim.addComponent(std::make_unique<NandGate>(), {"A", "n1"}, "n2");
    sim.addComponent(std::make_unique<NandGate>(), {"B", "n1"}, "n3");
    sim.addComponent(std::make_unique<NandGate>(), {"n2", "n3"}, "OUT");

    // Observer pattern: this lambda is called only when OUT actually
    // changes value — not once per event, not once per run().
    sim.addWatcher("OUT", [](bool value) {
        std::cout << "  [watcher] OUT changed to " << value << "\n";
    });

    std::cout << "-- Initial inputs: A=0, B=0 --\n";
    sim.scheduleInputChange(0, "A", false);
    sim.scheduleInputChange(0, "B", false);
    std::size_t evaluations = sim.run();
    std::cout << "OUT = " << sim.wireValue("OUT")
              << "  (" << evaluations << " component evaluations)\n\n";

    std::cout << "-- Flip B: 0 -> 1 --\n";
    sim.scheduleInputChange(100, "B", true);
    evaluations = sim.run();
    std::cout << "OUT = " << sim.wireValue("OUT")
              << "  (" << evaluations << " component evaluations)\n\n";

    std::cout << "-- Set B to the SAME value again (1 -> 1, no real change) --\n";
    sim.scheduleInputChange(200, "B", true);
    evaluations = sim.run();
    std::cout << "OUT = " << sim.wireValue("OUT")
              << "  (" << evaluations << " component evaluations — nothing changed, nothing ran)\n";

    return 0;
}
