#pragma once

#include <ostream>

// ============================================================================
// LogicValue.h  —  Phase 6 (Templates), part 1 of 2 (see LogicGate.h)
//
// Fixes the limitation Phase 4 ran into directly: every signal so far has
// been plain `bool`, so an "unset" wire is indistinguishable from a
// genuinely-computed `false` — real digital simulators avoid this with a
// third "unknown" state. LogicValue adds that (plus a fourth,
// high-impedance/"not driven" state), modeling the classic 4-valued
// logic real HDLs (VHDL's std_logic, Verilog's 4-state nets) use.
//
// This is a NEW, INDEPENDENT hierarchy — not built on Component/Gate, and
// doesn't touch them. It's a parallel technique to Phase 1/2's gates (the
// same relationship Phase 4 had to Phase 3): same gate *behaviors*
// (AND/OR/NOT/...), generalized to work over more than one value type.
// See docs/DECISIONS.md, "Phase 6 is independent, like Phase 2 and 4."
//
// Simplification, stated plainly: high-impedance (Z) is treated as
// equivalent to Unknown (X) for the logic operations below. Real HDLs
// give Z special treatment for tri-state bus resolution (multiple
// drivers, pull-up/down logic) — genuinely more than this project needs
// right now. Noted here, not hidden.
// ============================================================================

namespace eda {

enum class LogicValue {
    Zero,
    One,
    Unknown,
    HighZ
};

// Free-function overloads, NOT a template — see LogicGate.h for why this
// specific shape (overloads per concrete type, no generic primary
// template) is what makes the LogicType concept actually reject
// unsupported types instead of silently compiling something broken.
bool logicalAnd(bool a, bool b);
bool logicalOr(bool a, bool b);
bool logicalNot(bool a);

LogicValue logicalAnd(LogicValue a, LogicValue b);
LogicValue logicalOr(LogicValue a, LogicValue b);
LogicValue logicalNot(LogicValue a);

char toChar(LogicValue value);
std::ostream& operator<<(std::ostream& out, LogicValue value);

} // namespace eda
