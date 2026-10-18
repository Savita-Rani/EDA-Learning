#include "eda/Netlist.h"

#include <queue>
#include <stdexcept>
#include <utility>

// ============================================================================
// Netlist.cpp — Phase 3 implementation.
//
// evaluationOrder() is a textbook Kahn's-algorithm topological sort:
//  1. Figure out which entry (if any) PRODUCES each wire.
//  2. For every entry, its "dependencies" are the entries producing its
//     input wires (an input wire with no producer is an external input —
//     already available, not a dependency).
//  3. Entries with zero remaining dependencies are "ready." Process ready
//     entries one at a time; whenever processing one brings a dependent
//     entry's remaining-dependency count to zero, that entry becomes ready.
//  4. If every entry eventually gets processed, the order is valid. If
//     some never do, there's a cycle (a feedback loop) — not representable
//     as a combinational circuit, so we throw.
// ============================================================================

namespace eda {

void Netlist::addComponent(std::unique_ptr<Component> component,
                            std::vector<std::string> inputWires,
                            std::string outputWire) {
    wireValues_[outputWire]; // ensure the wire exists (default: false)
    entries_.push_back(Entry{std::move(component), std::move(inputWires), std::move(outputWire)});
}

void Netlist::setInput(const std::string& wireName, bool value) {
    wireValues_[wireName] = value;
}

std::vector<std::size_t> Netlist::evaluationOrder() const {
    const std::size_t n = entries_.size();

    // Which entry produces each wire (wires with no entry here are
    // external inputs).
    std::unordered_map<std::string, std::size_t> producerOf;
    for (std::size_t i = 0; i < n; ++i) {
        producerOf[entries_[i].outputWire] = i;
    }

    // dependents[p] = entries that depend on entry p's output.
    // inDegree[i] = how many not-yet-processed entries i depends on.
    std::vector<std::vector<std::size_t>> dependents(n);
    std::vector<std::size_t> inDegree(n, 0);

    for (std::size_t i = 0; i < n; ++i) {
        for (const std::string& inputWire : entries_[i].inputWires) {
            auto it = producerOf.find(inputWire);
            if (it != producerOf.end()) {
                dependents[it->second].push_back(i);
                ++inDegree[i];
            }
        }
    }

    std::queue<std::size_t> ready;
    for (std::size_t i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            ready.push(i);
        }
    }

    std::vector<std::size_t> order;
    order.reserve(n);
    while (!ready.empty()) {
        std::size_t current = ready.front();
        ready.pop();
        order.push_back(current);

        for (std::size_t dependent : dependents[current]) {
            if (--inDegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (order.size() != n) {
        throw std::runtime_error("Netlist has a feedback loop — no valid evaluation order");
    }

    return order;
}

void Netlist::simulate() {
    for (std::size_t idx : evaluationOrder()) {
        Entry& entry = entries_[idx];
        bool a = wireValue(entry.inputWires[0]);
        bool b = entry.inputWires.size() > 1 ? wireValue(entry.inputWires[1]) : false;
        wireValues_[entry.outputWire] = entry.component->evaluate(a, b);
    }
}

bool Netlist::wireValue(const std::string& wireName) const {
    auto it = wireValues_.find(wireName);
    return it != wireValues_.end() ? it->second : false;
}

} // namespace eda
