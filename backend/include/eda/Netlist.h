#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "eda/Component.h"

// ============================================================================
// Netlist.h  —  Phase 3 (STL)
//
// Builds ON TOP of Phase 2's Component (eda/Component.h) — reused as-is,
// not duplicated, not edited. This is a different relationship than
// Phase 2 had to Phase 1: Phase 2 was a deliberate *parallel*
// reimplementation of Gate for side-by-side comparison; Phase 3 *depends*
// on Component because the roadmap's own plan was "represent a netlist of
// Components" — Phase 2 built exactly the interface this phase needed.
// See docs/DECISIONS.md, "Phase 3 depends on Phase 2" for the full note.
//
// What a Netlist IS: a graph. Nodes are named wires (just std::string —
// see below for why there's no separate Wire/Signal class); edges are
// "this component reads wire X, writes wire Y." Some wires are external
// inputs (set directly via setInput, no component produces them); the
// rest are each produced by exactly one component.
//
// Concepts introduced here:
//  - std::unordered_map<std::string, bool>  : wire name -> current value.
//    Hash map, not a Wire/Signal class — a dedicated class would carry no
//    behavior beyond "name + value," so a map keeps this honest instead
//    of adding ceremony. (Named wires ARE the "Signal" concept from the
//    roadmap; this is a deliberately lighter-weight realization of it.)
//  - std::vector<Entry> for netlist storage, with Entry a private nested
//    struct — groups "one component + its wiring" without needing a
//    separate named type at namespace scope.
//  - std::unique_ptr<Component> ownership, same as Phase 2's frontend —
//    the Netlist OWNS every component you add to it.
//  - References vs pointers: `addComponent` takes `std::unique_ptr<Component>`
//    BY VALUE (transferring ownership via std::move at the call site) but
//    `inputWires`/`outputWire` by `const&` (read, not owned, not modified).
//  - Topological sort (Kahn's algorithm): `evaluationOrder()` figures out
//    a valid order to evaluate components in, such that every component's
//    inputs are already computed before it runs — using a queue, an
//    adjacency list, and in-degree counts, all built from STL containers.
// ============================================================================

namespace eda {

class Netlist {
public:
    // Adds a component wired to the given input wire name(s) (1 for a
    // NOT-shaped gate, 2 for everything else) and a single output wire
    // name. The Netlist takes ownership of `component`.
    void addComponent(std::unique_ptr<Component> component,
                       std::vector<std::string> inputWires,
                       std::string outputWire);

    // Sets an external input wire's value (one with no component
    // producing it) — call before simulate().
    void setInput(const std::string& wireName, bool value);

    // Returns indices into the internal entry list, in an order where
    // every entry appears after every entry that produces one of its
    // inputs. Throws std::runtime_error if the netlist has a feedback
    // loop (no valid order exists for a combinational circuit).
    std::vector<std::size_t> evaluationOrder() const;

    // Evaluates every component in topological order, updating every
    // wire's value along the way.
    void simulate();

    // Current value of any wire (input or component output). Returns
    // false for a wire that's never been set/produced.
    bool wireValue(const std::string& wireName) const;

private:
    struct Entry {
        std::unique_ptr<Component> component;
        std::vector<std::string> inputWires;
        std::string outputWire;
    };

    std::vector<Entry> entries_;
    std::unordered_map<std::string, bool> wireValues_;
};

} // namespace eda
