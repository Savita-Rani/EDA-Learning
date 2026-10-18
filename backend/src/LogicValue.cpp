#include "eda/LogicValue.h"

// ============================================================================
// LogicValue.cpp — Phase 6 implementation, part 1.
//
// Truth tables below follow standard 4-valued logic (Z folded into X, per
// the simplification noted in LogicValue.h):
//
// AND: 0 dominates (0 AND anything = 0); 1 AND 1 = 1; otherwise Unknown.
// OR:  1 dominates (1 OR anything = 1); 0 OR 0 = 0; otherwise Unknown.
// NOT: 0<->1 flip; Unknown/HighZ both invert to Unknown (still unknown).
// ============================================================================

namespace eda {

bool logicalAnd(bool a, bool b) { return a && b; }
bool logicalOr(bool a, bool b) { return a || b; }
bool logicalNot(bool a) { return !a; }

LogicValue logicalAnd(LogicValue a, LogicValue b) {
    if (a == LogicValue::Zero || b == LogicValue::Zero) {
        return LogicValue::Zero;
    }
    if (a == LogicValue::One && b == LogicValue::One) {
        return LogicValue::One;
    }
    return LogicValue::Unknown;
}

LogicValue logicalOr(LogicValue a, LogicValue b) {
    if (a == LogicValue::One || b == LogicValue::One) {
        return LogicValue::One;
    }
    if (a == LogicValue::Zero && b == LogicValue::Zero) {
        return LogicValue::Zero;
    }
    return LogicValue::Unknown;
}

LogicValue logicalNot(LogicValue a) {
    switch (a) {
        case LogicValue::Zero: return LogicValue::One;
        case LogicValue::One:  return LogicValue::Zero;
        case LogicValue::Unknown: return LogicValue::Unknown;
        case LogicValue::HighZ:   return LogicValue::Unknown;
    }
    return LogicValue::Unknown; // unreachable — every enumerator handled above
}

char toChar(LogicValue value) {
    switch (value) {
        case LogicValue::Zero:    return '0';
        case LogicValue::One:     return '1';
        case LogicValue::Unknown: return 'X';
        case LogicValue::HighZ:   return 'Z';
    }
    return '?'; // unreachable
}

std::ostream& operator<<(std::ostream& out, LogicValue value) {
    return out << toChar(value);
}

} // namespace eda
