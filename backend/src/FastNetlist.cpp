#include "eda/FastNetlist.h"

#include <queue>
#include <stdexcept>
#include <utility>

// ============================================================================
// FastNetlist.cpp — Phase 12 implementation.
//
// rebuildEvaluationOrder() is the same Kahn's-algorithm topological sort
// as Netlist::evaluationOrder() (Phase 3), just working over interned
// integer wire slots instead of std::string wire names, and cached
// instead of recomputed on every simulate() call. See FastNetlist.h for
// why both changes matter.
// ============================================================================

namespace eda {

std::size_t FastNetlist::internWire(const std::string& name) {
    auto it = wireIndex_.find(name);
    if (it != wireIndex_.end()) {
        return it->second;
    }
    const std::size_t index = wireValues_.size();
    wireIndex_.emplace(name, index);
    wireValues_.push_back(false);
    producerOf_.push_back(kNoProducer);
    return index;
}

void FastNetlist::addComponent(std::unique_ptr<Component> component,
                                std::vector<std::string> inputWires,
                                std::string outputWire) {
    Entry entry;
    entry.component = std::move(component);
    entry.inputSlot[0] = internWire(inputWires[0]);
    entry.hasSecondInput = inputWires.size() > 1;
    entry.inputSlot[1] = entry.hasSecondInput ? internWire(inputWires[1]) : 0;
    entry.outputSlot = internWire(outputWire);

    const std::size_t entryIndex = entries_.size();
    producerOf_[entry.outputSlot] = entryIndex;
    entries_.push_back(std::move(entry));

    // Topology changed — the cached order (if any) no longer applies.
    evaluationOrderValid_ = false;
}

void FastNetlist::setInput(const std::string& wireName, bool value) {
    wireValues_[internWire(wireName)] = value;
}

void FastNetlist::rebuildEvaluationOrder() {
    const std::size_t n = entries_.size();

    std::vector<std::vector<std::size_t>> dependents(n);
    std::vector<std::size_t> inDegree(n, 0);

    for (std::size_t i = 0; i < n; ++i) {
        const Entry& entry = entries_[i];
        const std::size_t inputCount = entry.hasSecondInput ? 2 : 1;
        for (std::size_t k = 0; k < inputCount; ++k) {
            const std::size_t producer = producerOf_[entry.inputSlot[k]];
            if (producer != kNoProducer) {
                dependents[producer].push_back(i);
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

    evaluationOrder_.clear();
    evaluationOrder_.reserve(n);
    while (!ready.empty()) {
        const std::size_t current = ready.front();
        ready.pop();
        evaluationOrder_.push_back(current);

        for (std::size_t dependent : dependents[current]) {
            if (--inDegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (evaluationOrder_.size() != n) {
        throw std::runtime_error("FastNetlist has a feedback loop — no valid evaluation order");
    }

    evaluationOrderValid_ = true;
}

void FastNetlist::simulate() {
    if (!evaluationOrderValid_) {
        rebuildEvaluationOrder();
    }

    for (std::size_t idx : evaluationOrder_) {
        Entry& entry = entries_[idx];
        const bool a = wireValues_[entry.inputSlot[0]];
        const bool b = entry.hasSecondInput ? wireValues_[entry.inputSlot[1]] : false;
        wireValues_[entry.outputSlot] = entry.component->evaluate(a, b);
    }
}

bool FastNetlist::wireValue(const std::string& wireName) const {
    auto it = wireIndex_.find(wireName);
    return it != wireIndex_.end() ? static_cast<bool>(wireValues_[it->second]) : false;
}

} // namespace eda
