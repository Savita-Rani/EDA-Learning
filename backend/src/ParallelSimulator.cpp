#include "eda/ParallelSimulator.h"

#include <stdexcept>
#include <thread>
#include <utility>

// ============================================================================
// ParallelSimulator.cpp — Phase 8 implementation, part 2.
// ============================================================================

namespace eda {

ParallelSimulator::ParallelSimulator(std::size_t numThreads, std::chrono::microseconds simulatedGateCost)
    : simulatedGateCost_(simulatedGateCost), pool_(numThreads) {}

void ParallelSimulator::addComponent(std::unique_ptr<Component> component,
                                      std::vector<std::string> inputWires,
                                      std::string outputWire) {
    std::lock_guard<std::mutex> lock(wireValuesMutex_);
    wireValues_[outputWire]; // ensure it exists (default: false)
    entries_.push_back(Entry{std::move(component), std::move(inputWires), std::move(outputWire)});
}

void ParallelSimulator::setInput(const std::string& wireName, bool value) {
    std::lock_guard<std::mutex> lock(wireValuesMutex_);
    wireValues_[wireName] = value;
}

std::vector<std::vector<std::size_t>> ParallelSimulator::computeLevels() const {
    const std::size_t n = entries_.size();

    std::unordered_map<std::string, std::size_t> producerOf;
    for (std::size_t i = 0; i < n; ++i) {
        producerOf[entries_[i].outputWire] = i;
    }

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

    std::vector<std::vector<std::size_t>> levels;
    std::vector<std::size_t> currentLevel;
    for (std::size_t i = 0; i < n; ++i) {
        if (inDegree[i] == 0) {
            currentLevel.push_back(i);
        }
    }

    std::size_t processed = 0;
    while (!currentLevel.empty()) {
        processed += currentLevel.size();
        std::vector<std::size_t> nextLevel;
        for (std::size_t idx : currentLevel) {
            for (std::size_t dependent : dependents[idx]) {
                if (--inDegree[dependent] == 0) {
                    nextLevel.push_back(dependent);
                }
            }
        }
        levels.push_back(currentLevel);
        currentLevel = std::move(nextLevel);
    }

    if (processed != n) {
        throw std::runtime_error("ParallelSimulator: netlist has a feedback loop — no valid level assignment");
    }

    return levels;
}

void ParallelSimulator::simulate() {
    evaluationCount_.store(0, std::memory_order_relaxed); // count THIS call only, per the header's documented contract
    std::vector<std::vector<std::size_t>> levels = computeLevels();

    for (const std::vector<std::size_t>& level : levels) {
        for (std::size_t entryIndex : level) {
            pool_.submit([this, entryIndex] { evaluateEntry(entryIndex); });
        }
        pool_.waitUntilIdle(); // barrier: next level must not start early
    }
}

void ParallelSimulator::evaluateEntry(std::size_t entryIndex) {
    const Entry& entry = entries_[entryIndex];

    bool a;
    bool b;
    {
        std::lock_guard<std::mutex> lock(wireValuesMutex_);
        a = wireValueLocked(entry.inputWires[0]);
        b = entry.inputWires.size() > 1 ? wireValueLocked(entry.inputWires[1]) : false;
    }

    if (simulatedGateCost_.count() > 0) {
        std::this_thread::sleep_for(simulatedGateCost_);
    }

    bool result = entry.component->evaluate(a, b);
    evaluationCount_.fetch_add(1, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(wireValuesMutex_);
    wireValues_[entry.outputWire] = result;
}

bool ParallelSimulator::wireValue(const std::string& wireName) const {
    std::lock_guard<std::mutex> lock(wireValuesMutex_);
    return wireValueLocked(wireName);
}

bool ParallelSimulator::wireValueLocked(const std::string& wireName) const {
    auto it = wireValues_.find(wireName);
    return it != wireValues_.end() ? it->second : false;
}

} // namespace eda
