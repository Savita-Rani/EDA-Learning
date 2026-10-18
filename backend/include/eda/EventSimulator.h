#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "eda/Component.h"

// ============================================================================
// EventSimulator.h  —  Phase 4 (Event-driven simulation engine)
//
// Depends on Phase 2's Component (reused, not duplicated — same as Phase
// 3). Deliberately does NOT depend on Phase 3's Netlist, and is not a
// modification of it: this is a second, independent technique for the
// SAME underlying problem (propagate values through a wired-up set of
// Components correctly), the same relationship Phase 2 had to Phase 1 —
// two implementations, side by side, so the tradeoff is visible instead
// of assumed. See docs/DECISIONS.md, "Phase 4 is a parallel technique,
// not a Netlist extension."
//
// The problem with Phase 3's Netlist::simulate(): it recomputes EVERY
// component, every single call, regardless of whether any of its inputs
// actually changed. Fine for 4 gates; wasteful for a large circuit where
// one input flip only affects a small downstream slice.
//
// EventSimulator instead only re-evaluates a component when a wire it
// actually reads has CHANGED value — and only propagates further when
// ITS output also changes. Nothing changes -> nothing downstream runs.
//
// Concepts introduced here:
//  - std::priority_queue<Event, ..., Comparator> : the event queue, a
//    min-heap ordered by scheduled time (smallest time processed first).
//    `std::priority_queue` is a max-heap by default — EventComparator
//    inverts the comparison to get min-heap behavior.
//  - std::function<void(bool)>     : the Observer pattern's callback type.
//    A "watcher" is just anything callable with a `bool` — a lambda, a
//    free function, a bound member function — EventSimulator doesn't
//    know or care which.
//  - lambdas                       : how calling code will actually
//    register watchers (see frontend/event_demo/main.cpp).
//  - Observer pattern               : wires notify their registered
//    watchers on an actual value change — decoupled from the propagation
//    logic that drives the simulation itself (two separate notification
//    paths: internal fanout re-evaluation, external watcher callbacks).
//  - "Glitches"/multiple evaluations before settling: a component can
//    still be re-evaluated more than once while a circuit "settles"
//    after an input change, passing through intermediate values that
//    aren't the final steady state — the same real phenomenon digital
//    designers call a "glitch." See EventSimulator.cpp for the batching
//    rule that keeps this from producing a WRONG final answer (it
//    originally didn't — see docs/phases/phase4.md's "a real bug" section
//    for the actual race condition this fixes and how it was found).
//  - std::unordered_set<std::size_t>  : used to deduplicate which
//    components need re-evaluating within one simulated time instant —
//    if two of a gate's inputs both change at the same time, it must be
//    evaluated exactly ONCE with both new values, not twice with
//    half-updated state in between.
// ============================================================================

namespace eda {

class EventSimulator {
public:
    // Same wiring shape as Netlist::addComponent — 1 input wire for a
    // NOT-shaped gate, 2 for everything else, one output wire.
    void addComponent(std::unique_ptr<Component> component,
                       std::vector<std::string> inputWires,
                       std::string outputWire);

    // Registers a callback invoked with the new value whenever `wireName`
    // actually changes (not on every event — only on a real transition).
    void addWatcher(const std::string& wireName, std::function<void(bool)> callback);

    // Schedules `wireName` to become `value` at simulation time `time`.
    // Used for external inputs; internal propagation schedules its own
    // events automatically.
    void scheduleInputChange(std::size_t time, std::string wireName, bool value);

    // Processes every currently queued event, and any further events
    // they generate, in time order, until the queue is empty. Returns
    // the number of component RE-EVALUATIONS actually performed — the
    // number that matters for contrasting against Phase 3's "evaluate
    // everything, every call."
    std::size_t run();

    bool wireValue(const std::string& wireName) const;

private:
    struct Entry {
        std::unique_ptr<Component> component;
        std::vector<std::string> inputWires;
        std::string outputWire;
    };

    struct Event {
        std::size_t time;
        std::string wireName;
        bool value;
    };

    struct EventComparator {
        bool operator()(const Event& a, const Event& b) const {
            return a.time > b.time; // smaller time = higher priority (min-heap)
        }
    };

    std::vector<Entry> entries_;

    // wire name -> indices of entries that read it as an input ("fanout").
    std::unordered_map<std::string, std::vector<std::size_t>> fanout_;

    std::unordered_map<std::string, bool> wireValues_;
    std::unordered_map<std::string, std::vector<std::function<void(bool)>>> watchers_;

    std::priority_queue<Event, std::vector<Event>, EventComparator> events_;
};

} // namespace eda
