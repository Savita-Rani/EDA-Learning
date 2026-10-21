#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "eda/Component.h"

// ============================================================================
// FastNetlist.h  —  Phase 12 (Performance)
//
// A second, INDEPENDENT implementation of the same problem Phase 3's
// Netlist (eda/Netlist.h) solves — same public shape (addComponent/
// setInput/simulate/wireValue), but NOT built on top of Netlist and NOT
// an edit to it (that file is Phase 3's and stays untouched). A parallel
// technique for side-by-side comparison, the same relationship Phase 4's
// EventSimulator and Phase 6's LogicGate<T> already have to earlier
// phases — see docs/DECISIONS.md, "Phase 12 is a parallel technique, not
// a Netlist edit."
//
// WHY THIS EXISTS: profiling Netlist::simulate() under repeated calls
// (gprof, real numbers in docs/phases/phase12.md) found two real,
// measured costs that Phase 3 never had reason to care about, but that
// dominate once simulate() runs thousands of times on the same circuit
// shape — exactly the pattern a clocked simulation loop produces:
//
//   1. Netlist::simulate() calls evaluationOrder() EVERY time, re-running
//      Kahn's algorithm from scratch — even though the circuit's
//      TOPOLOGY (which component produces which wire) never changes
//      between calls, only wire VALUES do.
//   2. Every wire read/write hashes a std::string
//      (unordered_map<std::string, bool>) — once per input per component
//      per simulate() call.
//
// FastNetlist fixes both:
//   - Wire names are interned to small integer indices ONCE, the first
//     time addComponent()/setInput() sees them. Every simulate() call
//     after that reads/writes a std::vector<bool> by index — no string
//     hashing anywhere in the hot loop.
//   - The evaluation order is computed once (lazily, on the first
//     simulate() call after a topology change) and reused on every
//     subsequent simulate() call, until the next addComponent()
//     invalidates the cache.
//
// Concepts: lazy caching behind an explicit "dirty" flag, replacing a
// string key with an interned integer index (a classic cache-friendly-
// layout technique — contiguous std::vector<bool> access instead of
// scattered hash-map buckets), and the general "compile once, run many
// times" idea real EDA tools and compilers rely on.
// ============================================================================

namespace eda {

class FastNetlist {
public:
    // Same contract as Netlist::addComponent: 1 input wire for a
    // NOT-shaped gate, 2 for everything else; takes ownership of
    // `component`.
    void addComponent(std::unique_ptr<Component> component,
                       std::vector<std::string> inputWires,
                       std::string outputWire);

    // Sets an external input wire's value — call before simulate().
    void setInput(const std::string& wireName, bool value);

    // Reuses the cached evaluation order if the topology hasn't changed
    // since the last call; rebuilds it otherwise. Throws
    // std::runtime_error on a feedback loop, same as Netlist.
    void simulate();

    // Current value of any wire. Returns false for a wire never
    // set/produced, same as Netlist.
    bool wireValue(const std::string& wireName) const;

private:
    struct Entry {
        std::unique_ptr<Component> component;
        std::size_t inputSlot[2];
        bool hasSecondInput;
        std::size_t outputSlot;
    };

    static constexpr std::size_t kNoProducer = static_cast<std::size_t>(-1);

    std::size_t internWire(const std::string& name);
    void rebuildEvaluationOrder();

    std::unordered_map<std::string, std::size_t> wireIndex_;
    std::vector<bool> wireValues_;
    std::vector<std::size_t> producerOf_; // wire slot -> entry index, or kNoProducer
    std::vector<Entry> entries_;
    std::vector<std::size_t> evaluationOrder_;
    bool evaluationOrderValid_ = false;
};

} // namespace eda
