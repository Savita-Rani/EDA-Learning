#pragma once

// ============================================================================
// Gate.h  —  Phase 1 (Basics)
//
// Concepts introduced here:
//  - #pragma once          : a header guard; stops this file being pasted
//                             into a .cpp more than once by the preprocessor.
//  - enum class             : a strongly-typed enum. Unlike plain C `enum`,
//                             GateType::AND will NOT silently convert to int,
//                             and won't collide with names in other enums.
//  - class vs struct        : classes default to `private` members, structs
//                             default to `public`. We use `class` here because
//                             GateType is implementation detail; evaluate()
//                             is the public interface.
//  - declaration vs definition: this file only *declares* what a Gate is and
//                             what it can do. The actual code (the "how")
//                             lives in Gate.cpp. This split lets other files
//                             #include this header without recompiling the
//                             implementation every time it changes.
// ============================================================================

enum class GateType {
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR
};

class Gate {
public:
    // Constructor: builds a Gate of a given type. `explicit` stops accidental
    // implicit conversions like `Gate g = GateType::AND;`
    explicit Gate(GateType type);

    // Evaluate the gate's output given its input(s).
    // For a NOT gate, only `a` is used; `b` is ignored.
    // `const` means this function promises not to modify the Gate object.
    bool evaluate(bool a, bool b = false) const;

    // Returns a short human-readable name, e.g. "AND". Useful for printing.
    const char* name() const;

private:
    GateType type_;   // trailing underscore: our convention for member data
};
