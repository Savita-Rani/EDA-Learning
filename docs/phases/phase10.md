# Phase 10 — Design patterns

**Goal:** fix Phase 9's leftover limitation — the architecture (how
gates/wires/the engine relate) has accreted informally across 9 phases,
never named or cleaned up as patterns. Three GoF patterns, formalized:
**Factory** (`ComponentFactory`), **Visitor** (`ComponentVisitor`), and
a **refined Observer** (`Signal<Args...>`, generalizing Phase 4's
`EventSimulator::addWatcher`).

**Status:** done. Builds clean, demo output correct for all three
patterns, GoogleTest suite green (16/16 new, 86/86 total across all ten
phases). No bug found this time — joining Phases 6 and 9 in that
category (see `docs/phases/phase6.md`'s "What's different about this
phase" for why that's worth stating honestly rather than implying every
phase must turn one up).

## A real constraint this phase had to design around

The textbook GoF Visitor pattern needs a virtual `accept(Visitor&)`
method on the **visited** class, which calls back `visitor.visit(*this)`
— "double dispatch." `Component` is Phase 2's file. Off-limits.

This is exactly the real-world situation "Visitor via external
dispatch" exists for: you need Visitor-shaped behavior over a
hierarchy you don't own (a third-party library's types — or, here, an
earlier phase's untouchable `Component`). Instead of
`component.accept(visitor)`, a free function `acceptVisitor()` does the
dispatch itself, via a `dynamic_cast` chain against each known concrete
type — RTTI, available for free since `Component` is already
polymorphic (virtual destructor, declared back in Phase 2). This isn't
a workaround snuck past the rule; it's the correct, standard technique
for this exact situation, and arguably more honest about *why* Visitor
sometimes needs this shape than a textbook example with a conveniently
editable base class would be.

## Relationship to earlier phases

Split by dependency shape, same pattern as Phases 8 and 9:

- **`eda_patterns`** (`BufferGate`, `ComponentFactory`,
  `ComponentVisitor`) — depends on Phase 2's `Component` (reused, like
  Phases 3/4/6/8/9). New capability (factory, traversal) built on an
  existing piece.
- **`eda_signal`** (`Signal<Args...>`) — fully independent, generic,
  header-only (no `.cpp` — `INTERFACE` library, same shape as Phase 9's
  `eda_waveform`).

`BufferGate` is a brand-new `Component` **subclass** — deriving
further from Phase 2's base class is reuse (exactly what `AndGate` etc.
already do), not an edit to `Component.h` itself.

**Deliberately not a retrofit:** Phase 5's `NetlistLoader.cpp` has its
own local, hardcoded, non-extensible factory function
(`makeComponent()`) — the exact ad hoc pattern `ComponentFactory`
formalizes. `NetlistLoader.cpp` is left completely untouched;
`ComponentFactory` is a fresh, independent formalization demonstrated
on its own, not a replacement wired into Phase 5's code. See
`docs/DECISIONS.md`, "ComponentFactory formalizes, doesn't replace,
Phase 5's ad hoc factory."

Nothing in any earlier phase's files was touched. The three edited
`CMakeLists.txt` got 66 insertions, 0 deletions.

## Files

- `backend/include/eda/BufferGate.h` / `.cpp` — a genuinely new gate
  type (single-input passthrough), existing specifically so `Factory`
  has something real to register beyond the 6 types every prior demo
  already had.
- `backend/include/eda/ComponentFactory.h` / `.cpp` — `registerType`/
  `create`/`isRegistered`/`registeredTypes`, plus `makeStandardGateFactory()`.
- `backend/include/eda/ComponentVisitor.h` / `.cpp` — the visitor
  interface + `acceptVisitor()`'s external dispatch.
- `backend/include/eda/Signal.h` — header-only variadic template.
- `frontend/patterns_demo/main.cpp` + `CMakeLists.txt` — all three,
  demonstrated together.
- `tests/test_buffer_gate.cpp`, `test_component_factory.cpp`,
  `test_component_visitor.cpp` (→ `eda_patterns_tests`),
  `test_signal.cpp` (→ `eda_signal_tests`) — 16 tests total.

## Factory: extensibility proven, not just claimed

`ComponentFactory::create`/`registerType` never mention `AndGate`,
`BufferGate`, or any concrete type by name in their own source — only
`makeStandardGateFactory()` does, and that function exists purely as a
convenience, not a requirement. The demo registers `BufferGate` (a type
that didn't exist in this project before this phase) at runtime,
without editing `ComponentFactory.cpp` at all:

```cpp
ComponentFactory factory = makeStandardGateFactory();
factory.registerType("BUFFER", [] { return std::make_unique<BufferGate>(); });
```

**Exception safety, demonstrated for real, not just asserted:**
`registerType` throws `std::invalid_argument` on a duplicate name
rather than silently overwriting the existing registration (a real bug
class in extensible factories — accidentally shadowing a type without
any error). The test `FailedRegistrationDoesNotCorruptExistingState`
specifically checks the strong guarantee: after a rejected
re-registration attempt throws and is caught, the **original**
registration for that name is still intact and correct — not
overwritten, not removed, not left half-updated.

## Visitor: the fallback path, exercised for real

`ComponentVisitor::visitUnknown()` exists for any `Component` subtype
`acceptVisitor()` doesn't recognize. Testing this honestly needs a type
genuinely outside the known list — `tests/test_component_visitor.cpp`
defines a tiny `MysteryGate` (existing *only* in that test file) and
confirms `acceptVisitor()` correctly routes it to `visitUnknown()`,
rather than just trusting the `else` branch compiles.

## Refined Observer: the same pattern, made reusable

Phase 4's `EventSimulator::addWatcher(wireName, std::function<void(bool)>)`
*is* the Observer pattern — but hardcoded for "a wire's `bool` value
changed." `Signal<Args...>` is what that same pattern looks like
generalized into a standalone, fully reusable primitive:

```cpp
Signal<bool> wireChanged;        // same shape EventSimulator's watchers have
Signal<std::string, int> renamed; // any shape at all — same class, zero changes
```

Both compile from the exact same `Signal<Args...>` definition — the
variadic template is what makes "any event shape" possible without
writing a new class per shape. `connect()` returns an opaque ID
specifically because `std::function` objects have no identity to
compare against (two "do the same thing" lambdas aren't `==`-comparable,
and `std::function` doesn't support `operator==` against another
`std::function` at all) — the ID is the only way to later
`disconnect()` one specific slot.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_patterns`, `eda_signal` (libraries),
`eda_patterns_demo` (frontend), `eda_patterns_tests`,
`eda_signal_tests` (tests).

## How to run

```powershell
./build/bin/eda_patterns_demo.exe
```

```
--- Factory: ComponentFactory ---
Registered types: XOR NAND NOT BUFFER NOR OR AND
  created AND: evaluate(true, false) = 0
  created BUFFER: evaluate(true, false) = 1
Attempting factory.create("UNKNOWN")...
  caught: ComponentFactory: no type registered under 'UNKNOWN'
Attempting to re-register "AND" (duplicate)...
  caught: ComponentFactory: 'AND' is already registered
  factory still works afterward -- creating AND again: AND

--- Visitor: ComponentVisitor (external dispatch, no accept() needed) ---
  AND=2 NAND=1 XOR=1 BUFFER=1 unknown=0

--- Refined Observer: Signal<Args...> ---
emit(true), emit(false) with both slots connected:
  [logger] wire changed to 1
  [logger] wire changed to 0
disconnected the logger; emit(true) again:
  counter slot saw 3 emissions total (logger saw fewer)
  [renamed] #3 is now called "OUT"
```

(Registered-types order varies — `std::unordered_map` has no ordering
guarantee; not a bug.)

## How to test

```powershell
cd build
ctest --output-on-failure
```

16 new tests, 86 total across all phases.

| Test | What it checks |
|---|---|
| `ComponentFactoryTest.CanRegisterANewTypeWithoutModifyingFactorySource` | the actual extensibility claim, exercised, not just asserted in a comment |
| `ComponentFactoryTest.FailedRegistrationDoesNotCorruptExistingState` | the strong exception-safety guarantee on a rejected `registerType()` |
| `ComponentVisitorTest.FallsBackToVisitUnknownForAnUnrecognizedType` | the fallback path, via a type (`MysteryGate`) that genuinely isn't in the dispatch chain |
| `ComponentVisitorTest.DefaultVisitorMethodsAreNoOpsNotCrashes` | a visitor using every default (empty) method doesn't crash on dispatch |
| `SignalTest.DisconnectStopsThatSlotWithoutAffectingOthers` | the ID-based disconnect mechanism's actual precision |
| `SignalTest.WorksWithMultipleArgumentsOfDifferentTypes` | the variadic template genuinely generalizes — not just `Signal<bool>` dressed up |

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues — nothing new toolchain-wise this phase.

- **Adding a new gate type and want the Visitor to recognize it** —
  add a `visitYourType()` method to `ComponentVisitor` and a
  corresponding `dynamic_cast` branch in `acceptVisitor()`. Until then
  it correctly falls through to `visitUnknown()` (that's the tested,
  intended behavior, not a gap).

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| Factory pattern, formalized and extensible | `ComponentFactory` | Decouples "what string maps to what type" from the factory's own source — `registerType()` lets any caller add types the factory was never written knowing about. |
| Visitor via external dispatch (not double-dispatch) | `ComponentVisitor`/`acceptVisitor()` | The correct technique specifically when you can't add `accept()` to the visited hierarchy — uses RTTI (`dynamic_cast`) instead of a virtual call on the visited object. |
| RTTI / polymorphic `dynamic_cast` | `acceptVisitor()` | Works because `Component` already has a virtual destructor (Phase 2) — any class with at least one virtual function is "polymorphic" and queryable this way. |
| Variadic templates (`template <typename... Args>`) | `Signal<Args...>` | One class definition serves `Signal<bool>`, `Signal<int,int>`, `Signal<std::string>`, etc. — the parameter pack *is* the generalization. |
| Opaque handle for later removal | `Signal::connect()`'s returned ID | `std::function` has no identity/equality to compare against; an explicit ID is the only way to `disconnect()` a *specific* slot later. |
| Strong exception-safety guarantee, tested | `ComponentFactory::registerType` | A failed call leaves the object's observable state completely unchanged — verified directly, not assumed from "well, it doesn't touch the map until the check passes." |

## Next up

**Phase 11 — Testing & tooling:** the codebase is now large enough
(10 phases, 86 tests) that manual spot-checking isn't sufficient
assurance of correctness on its own. GoogleTest already arrived early
(Phase 1) — this phase is about *expanding* coverage intentionally,
adding sanitizers (ASan/UBSan) to catch memory/UB bugs static review
can't, and CI scripting. See `docs/ROADMAP.md` → "Why this order" for
the full chain.
