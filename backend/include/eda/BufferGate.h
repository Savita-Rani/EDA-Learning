#pragma once

#include "eda/Component.h"

// ============================================================================
// BufferGate.h  —  Phase 10 (Design patterns)
//
// A brand new Component subclass — deriving further from Phase 2's
// Component is reuse (exactly what AndGate/OrGate/etc. already do),
// not an edit to Component.h itself. Exists specifically to give
// ComponentFactory something genuinely NEW to register at runtime —
// proof the factory is extensible without modifying the factory's own
// source or Component.h, not just a factory wrapping the same 6
// hardcoded types every earlier phase already had.
//
// A buffer gate is a real, simple digital component: single input,
// output equals input, unchanged. Used in real circuits to restore
// signal strength/timing without changing logic — included here
// because it's genuinely simple (one line) while being a type that
// didn't exist anywhere in this project before this phase.
// ============================================================================

namespace eda {

class BufferGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

} // namespace eda
