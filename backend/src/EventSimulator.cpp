#include "eda/EventSimulator.h"

#include <utility>

// ============================================================================
// EventSimulator.cpp — Phase 4 implementation.
//
// *** A real bug, found by testing, not assumed away ***
//
// The first version of this file processed ONE event at a time: pop an
// event, update its wire, immediately re-evaluate every component that
// reads it, push follow-up events, repeat. That has a genuine race: if
// TWO of a gate's inputs change at the exact same simulated time (e.g.
// two external inputs set together at time 0), each arrives as a
// SEPARATE event. Processing the first evaluates the gate using the
// OLD value of the second input (not yet applied) and schedules a
// result; processing the second evaluates the gate AGAIN using the
// updated value and schedules a DIFFERENT result — for the same wire,
// same future time. Whichever of those two events the priority queue
// happens to pop last silently overwrites the other, and nothing forces
// the correct one to win. This was caught by
// tests/test_event_simulator.cpp's `OnlyAffectedBranchEvaluates` test
// (an unrelated "branch B" wire flipped when only branch A's input
// changed) and a wrong final value for XOR(true, true) — not spotted by
// reasoning about the code, only by running it. See docs/phases/phase4.md.
//
// The fix: batch every event scheduled for the SAME time instant
// together. Apply all of their wire-value changes first, THEN decide
// which components actually need re-evaluating (deduplicated — a gate
// with two inputs changing in the same instant is evaluated once, with
// BOTH new values visible), THEN schedule next-time-step events from
// that fully-settled snapshot. This is "delta cycle" batching, the
// standard technique real event-driven HDL simulators use for exactly
// this reason.
// ============================================================================

namespace eda {

void EventSimulator::addComponent(std::unique_ptr<Component> component,
                                   std::vector<std::string> inputWires,
                                   std::string outputWire) {
    std::size_t index = entries_.size();

    for (const std::string& inputWire : inputWires) {
        fanout_[inputWire].push_back(index);
    }

    wireValues_[outputWire]; // ensure the wire exists (default: false)
    entries_.push_back(Entry{std::move(component), std::move(inputWires), std::move(outputWire)});
}

void EventSimulator::addWatcher(const std::string& wireName, std::function<void(bool)> callback) {
    watchers_[wireName].push_back(std::move(callback));
}

void EventSimulator::scheduleInputChange(std::size_t time, std::string wireName, bool value) {
    events_.push(Event{time, std::move(wireName), value});
}

std::size_t EventSimulator::run() {
    std::size_t evaluationCount = 0;

    while (!events_.empty()) {
        const std::size_t currentTime = events_.top().time;

        // --- Phase A: drain and apply every event at this exact time ---
        std::unordered_map<std::string, bool> changedThisInstant;
        while (!events_.empty() && events_.top().time == currentTime) {
            Event event = events_.top();
            events_.pop();

            auto it = wireValues_.find(event.wireName);
            bool unchanged = (it != wireValues_.end()) && (it->second == event.value);
            if (unchanged) {
                continue; // no real transition — nothing to propagate
            }

            wireValues_[event.wireName] = event.value;
            changedThisInstant[event.wireName] = event.value;
        }

        // --- Phase B: notify watchers for everything that just changed ---
        for (const auto& [wireName, value] : changedThisInstant) {
            auto watcherIt = watchers_.find(wireName);
            if (watcherIt != watchers_.end()) {
                for (const auto& callback : watcherIt->second) {
                    callback(value);
                }
            }
        }

        // --- Phase C: figure out which components are actually affected,
        //     ONCE each, even if more than one of their inputs changed
        //     in this same instant ---
        std::unordered_set<std::size_t> toEvaluate;
        for (const auto& [wireName, value] : changedThisInstant) {
            auto fanoutIt = fanout_.find(wireName);
            if (fanoutIt == fanout_.end()) {
                continue;
            }
            for (std::size_t entryIndex : fanoutIt->second) {
                toEvaluate.insert(entryIndex);
            }
        }

        // --- Phase D: evaluate each exactly once, using the now fully-
        //     settled wireValues_ for this instant, and schedule any
        //     resulting output change for the NEXT time step ---
        for (std::size_t entryIndex : toEvaluate) {
            Entry& entry = entries_[entryIndex];
            bool a = wireValue(entry.inputWires[0]);
            bool b = entry.inputWires.size() > 1 ? wireValue(entry.inputWires[1]) : false;
            bool newOutput = entry.component->evaluate(a, b);
            ++evaluationCount;
            events_.push(Event{currentTime + 1, entry.outputWire, newOutput});
        }
    }

    return evaluationCount;
}

bool EventSimulator::wireValue(const std::string& wireName) const {
    auto it = wireValues_.find(wireName);
    return it != wireValues_.end() ? it->second : false;
}

} // namespace eda
