#pragma once

#include <concepts>

#include "eda/LogicValue.h"

// ============================================================================
// LogicGate.h  —  Phase 6 (Templates), part 2 of 2
//
// Why this file has NO corresponding LogicGate.cpp, unlike every class in
// every previous phase: LogicGate<T> is a TEMPLATE. The compiler needs to
// see its full definition at every place it gets instantiated (e.g.
// `LogicGate<bool>`, `LogicGate<LogicValue>`) — a separately-compiled
// .cpp wouldn't be visible to those call sites. Templates are
// effectively header-only by necessity, not by style choice. This is a
// real, common point of confusion worth hitting directly: the
// header/source split taught since Phase 1 doesn't apply here, and this
// paragraph is why.
//
// Concepts introduced here:
//  - Class template `LogicGate<T>`     : ONE gate implementation, generic
//    over the signal value type T. `LogicGate<bool>` behaves exactly
//    like Phase 1's Gate/Phase 2's Component; `LogicGate<LogicValue>`
//    gets full 0/1/X/Z behavior — same code, different T.
//  - C++20 concept `LogicType`         : constrains T to "a type that has
//    logicalAnd/logicalOr/logicalNot overloads returning T." Only `bool`
//    and `LogicValue` satisfy it (see LogicValue.h's overloads — ADL
//    finds them). `LogicGate<int>` fails to COMPILE, with a message
//    naming exactly which constraint wasn't satisfied — not a cryptic
//    error from deep inside evaluate()'s body. That specific contrast is
//    the actual point of concepts over the old SFINAE/enable_if approach.
//  - Requires-expression                : `requires(T a, T b) { ... }` —
//    lists expressions that must be well-formed (and here, additionally
//    constrained to return exactly T via `-> std::same_as<T>`) for T to
//    satisfy the concept.
//  - Deriving NAND/NOR/XOR generically  : only AND/OR/NOT need a
//    per-type overload (matching the real hardware fact that AND+OR+NOT
//    — or NAND alone — is a "complete" logic basis, the same fact behind
//    Phase 3/4/5's 4-NAND XOR demo circuits). `evaluate()` composes them
//    itself, so support for a brand new value type requires writing
//    exactly 3 functions, not 6.
// ============================================================================

namespace eda {

template <typename T>
concept LogicType = requires(T a, T b) {
    { logicalAnd(a, b) } -> std::same_as<T>;
    { logicalOr(a, b) } -> std::same_as<T>;
    { logicalNot(a) } -> std::same_as<T>;
};

enum class GateKind {
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR
};

template <LogicType T>
class LogicGate {
public:
    explicit LogicGate(GateKind kind) : kind_(kind) {}

    T evaluate(T a, T b = T{}) const {
        switch (kind_) {
            case GateKind::AND:  return logicalAnd(a, b);
            case GateKind::OR:   return logicalOr(a, b);
            case GateKind::NOT:  return logicalNot(a);
            case GateKind::NAND: return logicalNot(logicalAnd(a, b));
            case GateKind::NOR:  return logicalNot(logicalOr(a, b));
            case GateKind::XOR:  return logicalOr(logicalAnd(a, logicalNot(b)),
                                                   logicalAnd(logicalNot(a), b));
        }
        return T{}; // unreachable — every GateKind handled above
    }

    const char* name() const {
        switch (kind_) {
            case GateKind::AND:  return "AND";
            case GateKind::OR:   return "OR";
            case GateKind::NOT:  return "NOT";
            case GateKind::NAND: return "NAND";
            case GateKind::NOR:  return "NOR";
            case GateKind::XOR:  return "XOR";
        }
        return "UNKNOWN";
    }

private:
    GateKind kind_;
};

// Uncommenting the next line fails to compile with a concept-violation
// diagnostic naming LogicType, pointing at this exact line — rather than
// a page of errors from inside evaluate()'s body:
//
//   LogicGate<int> wontCompile(GateKind::AND);
//
// Confirmed by actually compiling it: `int` implicitly converts to
// `bool`, so overload resolution picks the bool overload of
// logicalAnd/logicalOr/logicalNot rather than finding "no candidate" —
// but that overload returns `bool`, not `int`, so the concept's exact
// `-> std::same_as<T>` check still correctly fails. See
// docs/phases/phase6.md for the full captured compiler output.

} // namespace eda
