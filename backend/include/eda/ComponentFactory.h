#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "eda/Component.h"

// ============================================================================
// ComponentFactory.h  —  Phase 10 (Design patterns), part 1 of 3
//
// Formalizes a pattern this project already used informally: Phase 5's
// NetlistLoader.cpp has a local, static, hardcoded string->factory-
// function map (`makeComponent()`) baked directly into that file,
// covering exactly the 6 gate types known at the time it was written.
// ComponentFactory is what that ad hoc version "should grow into" —
// a standalone, REUSABLE, EXTENSIBLE factory: new types can be
// `registerType()`-ed by any caller at runtime, without editing this
// class's source OR Component.h.
//
// Deliberately NOT a retrofit — NetlistLoader.cpp (Phase 5) is left
// exactly as it was; this is a fresh, independent formalization of the
// same pattern, not an edit to how Phase 5 already works. See
// docs/DECISIONS.md, "ComponentFactory formalizes, doesn't replace,
// Phase 5's ad hoc factory."
//
// Depends on Phase 2's Component (reused, like Phases 3/4/6/8/9) — new
// capability (a reusable factory) built on an existing piece.
//
// Concepts introduced here:
//  - The Factory pattern, named and made reusable: callers decide what
//    string maps to what concrete type, at runtime, via registerType()
//    — this class never needs to know about AndGate/BufferGate/or any
//    future type by name.
//  - Exception safety as a stated contract, not an accident: create()
//    either returns a valid object or throws — no partial/corrupted
//    state either way (trivially true here since create() never
//    mutates the factory). registerType() throws on a duplicate name
//    specifically to prevent silently shadowing an existing
//    registration — a real bug class in extensible factories,
//    reasoned about explicitly rather than "whatever's convenient."
// ============================================================================

namespace eda {

class ComponentFactory {
public:
    using Creator = std::function<std::unique_ptr<Component>()>;

    // Registers `creator` under `name`. Throws std::invalid_argument if
    // `name` is already registered — silently overwriting an existing
    // registration is exactly the kind of mistake worth refusing
    // outright rather than letting the last caller quietly win.
    void registerType(const std::string& name, Creator creator);

    // Constructs a new Component of the type registered under `name`.
    // Throws std::out_of_range if `name` was never registered.
    std::unique_ptr<Component> create(const std::string& name) const;

    bool isRegistered(const std::string& name) const noexcept;
    std::vector<std::string> registeredTypes() const;

private:
    std::unordered_map<std::string, Creator> creators_;
};

// Convenience: a factory pre-populated with the 6 standard gates
// (AND/OR/NOT/NAND/NOR/XOR) from Phase 2. Callers are free to
// registerType() further on the result — e.g. BufferGate, or any
// future type — without touching this function's source.
ComponentFactory makeStandardGateFactory();

} // namespace eda
