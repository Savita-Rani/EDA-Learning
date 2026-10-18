#pragma once

#include "eda/BufferGate.h"
#include "eda/Component.h"

// ============================================================================
// ComponentVisitor.h  —  Phase 10 (Design patterns), part 2 of 3
//
// The textbook GoF Visitor pattern adds a virtual `accept(Visitor&)`
// method to the VISITED class, which then calls back `visitor.visit(*this)`
// — "double dispatch." That requires editing the visited hierarchy's
// base class. Component.h is Phase 2's file — off-limits here by this
// project's own rule (never edit a previous phase's files).
//
// This is exactly the real-world situation Visitor-via-external-
// dispatch exists for: when you need Visitor-style behavior over a
// class hierarchy you don't own (a third-party library's types, or —
// here — an earlier phase's untouchable Component). Instead of
// `component.accept(visitor)`, a free function `acceptVisitor()` below
// does the dispatch itself, externally, via `dynamic_cast` against each
// known concrete type. One extra requirement this needs that double-
// dispatch wouldn't: RTTI — already available for free, since
// Component already has a virtual destructor (declared in Phase 2),
// making it a polymorphic type `dynamic_cast` can query.
//
// Depends on Phase 2's Component AND Phase 10's own BufferGate
// (reused) — new capability (traversal) built on existing pieces.
// ============================================================================

namespace eda {

class ComponentVisitor {
public:
    virtual ~ComponentVisitor() = default;

    virtual void visitAnd(const AndGate&) {}
    virtual void visitOr(const OrGate&) {}
    virtual void visitNot(const NotGate&) {}
    virtual void visitNand(const NandGate&) {}
    virtual void visitNor(const NorGate&) {}
    virtual void visitXor(const XorGate&) {}
    virtual void visitBuffer(const BufferGate&) {}

    // Called for any Component whose concrete type acceptVisitor()
    // doesn't recognize — e.g. a type defined after this header, or
    // one from outside this project entirely.
    virtual void visitUnknown(const Component&) {}
};

// Dispatches `component` to the correctly-typed visitXxx() method via
// a dynamic_cast chain, falling back to visitUnknown() if the concrete
// type isn't one of the known gates.
void acceptVisitor(const Component& component, ComponentVisitor& visitor);

} // namespace eda
