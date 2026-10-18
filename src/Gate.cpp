#include "Gate.h"

// ============================================================================
// Gate.cpp — implementation (the "how") for the Gate class declared in
// Gate.h (the "what"). The compiler turns this file into Gate.o/Gate.obj
// independently of main.cpp; the linker then stitches the pieces together.
// ============================================================================

Gate::Gate(GateType type) : type_(type) {
    // Member initializer list `: type_(type)` runs BEFORE the constructor
    // body. Prefer it over assigning inside `{}` — it initializes directly
    // instead of default-constructing then reassigning.
}

bool Gate::evaluate(bool a, bool b) const {
    switch (type_) {
        case GateType::AND:  return a && b;
        case GateType::OR:   return a || b;
        case GateType::NOT:  return !a;
        case GateType::NAND: return !(a && b);
        case GateType::NOR:  return !(a || b);
        case GateType::XOR:  return a != b;
    }
    // Unreachable if every enumerator is handled above, but some compilers
    // still warn about a missing return without this.
    return false;
}

const char* Gate::name() const {
    switch (type_) {
        case GateType::AND:  return "AND";
        case GateType::OR:   return "OR";
        case GateType::NOT:  return "NOT";
        case GateType::NAND: return "NAND";
        case GateType::NOR:  return "NOR";
        case GateType::XOR:  return "XOR";
    }
    return "UNKNOWN";
}
