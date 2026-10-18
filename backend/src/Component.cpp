#include "eda/Component.h"

// ============================================================================
// Component.cpp — Phase 2 implementation.
//
// Notice there's no `switch` anywhere in this file. Each class only knows
// how to be ITSELF — AndGate::evaluate() only ever computes AND. The
// "which gate is this?" decision that Gate.cpp made explicitly with a
// `switch` on GateType now happens implicitly: whichever concrete object a
// Component& actually refers to at runtime determines which override runs.
// That's dynamic dispatch — looked up through each object's vtable, a
// hidden pointer-to-a-table-of-function-pointers the compiler generates
// per polymorphic class.
// ============================================================================

namespace eda {

bool AndGate::evaluate(bool a, bool b) const { return a && b; }
const char* AndGate::name() const { return "AND"; }

bool OrGate::evaluate(bool a, bool b) const { return a || b; }
const char* OrGate::name() const { return "OR"; }

bool NotGate::evaluate(bool a, bool /*b*/) const { return !a; }
const char* NotGate::name() const { return "NOT"; }

bool NandGate::evaluate(bool a, bool b) const { return !(a && b); }
const char* NandGate::name() const { return "NAND"; }

bool NorGate::evaluate(bool a, bool b) const { return !(a || b); }
const char* NorGate::name() const { return "NOR"; }

bool XorGate::evaluate(bool a, bool b) const { return a != b; }
const char* XorGate::name() const { return "XOR"; }

} // namespace eda
