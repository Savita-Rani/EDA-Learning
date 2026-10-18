#include <chrono>
#include <iostream>
#include <memory>
#include <eda/ParallelSimulator.h>

// ============================================================================
// main.cpp — console driver for Phase 8.
//
// Two demonstrations:
//  1. Correctness: the same 4-NAND XOR circuit as Phases 3/4/5's demos,
//     run through ParallelSimulator (multiple threads, zero artificial
//     delay) — must produce the same truth table.
//  2. Speedup: 8 completely independent AND gates (all in one
//     dependency level — "embarrassingly parallel"), each given an
//     ARTIFICIAL simulated cost (our real gates are far too cheap to
//     show a genuine wall-clock difference; thread/lock overhead would
//     dominate). Run once with 1 thread, once with 8, compare times.
//     Labeled clearly as artificial — this is NOT a claim about this
//     project's actual gate performance.
// ============================================================================

using namespace eda;
using Clock = std::chrono::steady_clock;

namespace {

void demonstrateCorrectness() {
    std::cout << "\n--- Correctness: 4-NAND XOR, 4 worker threads ---\n";
    std::cout << "  A  B  | OUT\n  -------------\n";

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

            std::cout << "  " << a << "  " << b << "  |  " << sim.wireValue("OUT") << "\n";
        }
    }
}

double buildAndRunEightIndependentGates(std::size_t numThreads, std::chrono::microseconds gateCost) {
    ParallelSimulator sim(numThreads, gateCost);
    for (int i = 0; i < 8; ++i) {
        std::string suffix = std::to_string(i);
        sim.addComponent(std::make_unique<AndGate>(), {"x" + suffix, "y" + suffix}, "out" + suffix);
        sim.setInput("x" + suffix, true);
        sim.setInput("y" + suffix, true);
    }

    auto start = Clock::now();
    sim.simulate();
    auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

    std::cout << "  evaluations performed: " << sim.evaluationCount() << "\n";
    return elapsed;
}

void demonstrateSpeedup() {
    std::cout << "\n--- Speedup (artificial gate cost — see comment in main.cpp) ---\n";
    constexpr auto kArtificialGateCost = std::chrono::microseconds(20000); // 20ms per gate

    std::cout << "8 independent gates, 1 worker thread:\n";
    double serialMs = buildAndRunEightIndependentGates(1, kArtificialGateCost);
    std::cout << "  elapsed: " << serialMs << " ms\n";

    std::cout << "8 independent gates, 8 worker threads:\n";
    double parallelMs = buildAndRunEightIndependentGates(8, kArtificialGateCost);
    std::cout << "  elapsed: " << parallelMs << " ms\n";

    std::cout << "speedup: " << (serialMs / parallelMs) << "x\n";
}

} // namespace

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 8: Concurrency ===\n";

    demonstrateCorrectness();
    demonstrateSpeedup();

    return 0;
}
