#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "eda/Component.h"
#include "eda/ThreadPool.h"

// ============================================================================
// ParallelSimulator.h  —  Phase 8 (Concurrency), part 2 of 2
//
// Depends on Phase 2's Component (reused, like Phases 3/4/6) AND on
// this phase's own ThreadPool — new capability (parallel evaluation)
// built on two existing pieces. Not a parallel-technique-vs-Netlist
// situation the way Phase 4 was to Phase 3: this solves the SAME
// "compute every wire's value" problem Netlist does, but via a
// DIFFERENT axis of improvement than Phase 4. Phase 4 made the engine
// do LESS work (only recompute what changed — incremental). This phase
// makes INDEPENDENT work happen CONCURRENTLY instead of serially — full
// recompute every simulate() call, same as Netlist, just parallelized.
// See docs/DECISIONS.md, "ParallelSimulator depends on Component and
// ThreadPool."
//
// How it finds independent work: the same topological-sort idea
// Netlist.cpp uses (Kahn's algorithm), but grouping entries into
// LEVELS instead of a flat order — level 0 has no dependencies at all;
// level k's entries depend only on levels < k. Every entry WITHIN one
// level is independent of every other entry in that same level (by
// construction), so they can all run concurrently; levels themselves
// must run strictly in order (a full ThreadPool::waitUntilIdle()
// barrier between them).
//
// Concepts introduced here:
//  - std::mutex guarding shared state  : every read/write of
//    `wireValues_` during simulate() happens under `wireValuesMutex_` —
//    multiple worker threads read/write a shared std::unordered_map
//    concurrently otherwise, which is undefined behavior regardless of
//    whether they touch different keys.
//  - std::atomic<std::size_t>          : `evaluationCount_` is
//    incremented by many worker threads concurrently with no mutex at
//    all — a plain `std::size_t` here would be a genuine data race
//    (lost increments); `fetch_add` is the lightweight, lock-free
//    alternative to a mutex for a simple shared counter.
//  - Simulated per-gate cost           : `simulatedGateCost` is an
//    honest acknowledgment that this project's actual gates (a few
//    boolean ops) are too cheap to show a REAL wall-clock speedup from
//    threading — thread/lock overhead would dominate. Defaults to zero
//    (real speed, used by tests); the demo sets it to something
//    artificially larger specifically to make the parallel-vs-serial
//    timing difference visible and honestly labeled as artificial.
// ============================================================================

namespace eda {

class ParallelSimulator {
public:
    explicit ParallelSimulator(std::size_t numThreads,
                                std::chrono::microseconds simulatedGateCost = std::chrono::microseconds(0));

    void addComponent(std::unique_ptr<Component> component,
                       std::vector<std::string> inputWires,
                       std::string outputWire);

    void setInput(const std::string& wireName, bool value);

    // Recomputes EVERY wire from scratch (like Netlist::simulate(),
    // unlike EventSimulator's incremental approach), evaluating each
    // dependency level's components concurrently via the ThreadPool.
    // Throws std::runtime_error on a feedback loop (no valid level
    // assignment exists), same condition Netlist detects.
    void simulate();

    bool wireValue(const std::string& wireName) const;

    // How many Component::evaluate() calls the most recent simulate()
    // performed — incremented from multiple threads via std::atomic.
    std::size_t evaluationCount() const noexcept { return evaluationCount_.load(); }

private:
    struct Entry {
        std::unique_ptr<Component> component;
        std::vector<std::string> inputWires;
        std::string outputWire;
    };

    std::vector<std::vector<std::size_t>> computeLevels() const;
    void evaluateEntry(std::size_t entryIndex);
    bool wireValueLocked(const std::string& wireName) const;

    std::vector<Entry> entries_;
    std::unordered_map<std::string, bool> wireValues_;
    mutable std::mutex wireValuesMutex_;
    std::atomic<std::size_t> evaluationCount_{0};
    std::chrono::microseconds simulatedGateCost_;
    ThreadPool pool_;
};

} // namespace eda
