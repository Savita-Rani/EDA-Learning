#pragma once

// ============================================================================
// Component.h  —  Phase 2 (OOP)
//
// Phase 1's Gate (eda/Gate.h) is left completely untouched — this is a new,
// separate hierarchy covering the same ground (AND/OR/NOT/NAND/NOR/XOR) a
// different way, so the two can be compared side by side:
//   Phase 1: ONE class, a `switch` over a GateType enum picks the behavior.
//   Phase 2: ONE base class + SIX derived classes, the compiler picks the
//            right evaluate() via a vtable lookup at runtime — polymorphism.
//
// Concepts introduced here:
//  - Abstract base class       : `Component` can never be instantiated
//                                 directly — it declares pure virtual
//                                 functions (`= 0`) with no body. It exists
//                                 only to be inherited from.
//  - Pure virtual functions     : `virtual ... = 0;` — every concrete
//                                 derived class MUST provide its own
//                                 evaluate()/name(), or it's abstract too.
//  - Virtual destructor         : `virtual ~Component() = default;` — if we
//                                 ever `delete` a Component* that's actually
//                                 pointing at an AndGate, a non-virtual
//                                 destructor would only run ~Component(),
//                                 skipping ~AndGate() (undefined behavior for
//                                 non-trivial derived members). Virtual fixes
//                                 that: the right destructor runs.
//  - `: public Component`       : public inheritance — "an AndGate IS-A
//                                 Component." Callers can hold an
//                                 `AndGate` through a `Component&`/`Component*`
//                                 and use it without knowing the concrete type.
//  - `override`                 : tells the compiler "this must match a
//                                 virtual function in the base" — catches
//                                 typos (wrong signature) as a compile error
//                                 instead of silently creating an unrelated
//                                 new function.
//  - `std::unique_ptr<Component>` : how main.cpp will OWN these objects —
//                                 previewed here, covered properly in
//                                 Phase 7 (Memory management). Short version:
//                                 Component is abstract, so it can't be
//                                 stored by value in a container — some form
//                                 of pointer is unavoidable, and a raw
//                                 `new`/`delete` pair is exactly the kind of
//                                 manual bookkeeping unique_ptr exists to
//                                 remove.
// ============================================================================

namespace eda {

class Component {
public:
    virtual ~Component() = default;

    // = 0 means "no implementation here, every concrete subclass must
    // supply one." Attempting `Component c;` is a compile error.
    virtual bool evaluate(bool a, bool b = false) const = 0;
    virtual const char* name() const = 0;
};

class AndGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

class OrGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

class NotGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

class NandGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

class NorGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

class XorGate : public Component {
public:
    bool evaluate(bool a, bool b = false) const override;
    const char* name() const override;
};

} // namespace eda
