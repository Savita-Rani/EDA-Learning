#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <eda/FastNetlist.h>
#include <eda/Netlist.h>

// ============================================================================
// netlist_bench.cpp — Phase 12 (Performance).
//
// Builds a dependency CHAIN of NotGates (wire0 -> NOT -> wire1 -> NOT ->
// wire2 -> ... ), the worst case for Netlist's per-simulate() topological
// sort (every entry depends on exactly the previous one — no shortcuts),
// then calls simulate() repeatedly on the SAME netlist instance — the
// pattern a clocked simulation loop produces, and the pattern that
// exposes the cost Netlist pays on every call that FastNetlist pays only
// once. See docs/phases/phase12.md for the captured numbers.
//
// Usage: eda_netlist_bench [chainLength] [iterations]
// ============================================================================

using namespace eda;
using Clock = std::chrono::steady_clock;

namespace {

template <typename NetlistType>
double benchmarkRepeatedSimulate(std::size_t chainLength, std::size_t iterations) {
    NetlistType netlist;
    for (std::size_t i = 0; i < chainLength; ++i) {
        netlist.addComponent(std::make_unique<NotGate>(),
                              {"w" + std::to_string(i)},
                              "w" + std::to_string(i + 1));
    }
    netlist.setInput("w0", true);

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        netlist.simulate();
    }
    const auto end = Clock::now();

    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main(int argc, char** argv) {
    const std::size_t chainLength = argc > 1 ? std::stoul(argv[1]) : 500;
    const std::size_t iterations = argc > 2 ? std::stoul(argv[2]) : 20000;

    std::cout << "chain length: " << chainLength
              << ", simulate() calls: " << iterations << "\n\n";

    const double netlistMs = benchmarkRepeatedSimulate<Netlist>(chainLength, iterations);
    std::cout << "Netlist     (Phase 3):  " << netlistMs << " ms\n";

    const double fastMs = benchmarkRepeatedSimulate<FastNetlist>(chainLength, iterations);
    std::cout << "FastNetlist (Phase 12): " << fastMs << " ms\n";

    std::cout << "\nspeedup: " << (netlistMs / fastMs) << "x\n";
    return 0;
}
