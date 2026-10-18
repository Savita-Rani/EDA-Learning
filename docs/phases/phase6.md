# Phase 6 — Templates & generic programming

**Goal:** fix the limitation every prior phase using `bool` signals
shared (and Phase 4 hit concretely): no way to represent "unknown" or
"not driven" — an unset wire is indistinguishable from a genuinely
computed `false`. `LogicValue` adds a real 0/1/X/Z type; `LogicGate<T>`
is ONE generic gate implementation that works for `bool` *or*
`LogicValue` (or any future type satisfying the same constraint), via
C++20 templates and concepts.

**Status:** done. Builds clean (including the concept-constrained
template — no warnings), demo output correct for both instantiations,
GoogleTest suite green (10/10 new, 38/38 total across all six phases).
No bugs found this time — unlike Phases 4 and 5, this one worked on the
first real build, which is its own useful data point (see "What's
different about this phase," below).

## Relationship to earlier phases

`LogicValue`/`LogicGate<T>` are a **new, independent** hierarchy — not
built on `Component`/`Gate`, and don't touch them. This is the
Phase-4-to-Phase-3 pattern (parallel technique), not the
Phase-3-to-Phase-2 pattern (reuse): `LogicGate<bool>` solves the *exact
same problem* Phase 1's `Gate` and Phase 2's `Component` solve, just
generalized. See `docs/DECISIONS.md`, "Phase 6 is independent, like
Phase 2 and 4."

Nothing in `Component.h`/`.cpp`, `Gate.h`/`.cpp`, `Netlist.h`/`.cpp`,
`EventSimulator.h`/`.cpp`, `NetlistLoader.h`/`.cpp`, or any earlier
frontend/test file was touched. The three edited `CMakeLists.txt` got 44
insertions, 0 deletions.

## Files

- `backend/include/eda/LogicValue.h` / `.cpp` — the `LogicValue` enum
  (`Zero`/`One`/`Unknown`/`HighZ`) and free-function overloads
  (`logicalAnd`/`logicalOr`/`logicalNot`) for both `bool` and
  `LogicValue`.
- `backend/include/eda/LogicGate.h` — **header-only** (no `.cpp` — see
  "Why no LogicGate.cpp" below). The `LogicType` concept and the
  `LogicGate<T>` class template.
- `frontend/generic_demo/main.cpp` + `CMakeLists.txt` — prints truth
  tables for `LogicGate<bool>` and `LogicGate<LogicValue>`.
- `tests/test_logic_value.cpp` + `test_logic_gate.cpp` — 10 GoogleTest
  cases total, new `eda_generic_tests` target (one executable, two
  source files — same pattern as any multi-file project).

## Why no `LogicGate.cpp`

Every class in Phases 1-5 split declaration (`.h`) from definition
(`.cpp`) — the compiler only needs the full body at the single place
it's compiled, once, into a library. Templates break this: the compiler
needs `LogicGate<T>`'s *complete* definition at **every** place it gets
instantiated (`LogicGate<bool>` in one file, `LogicGate<LogicValue>` in
another) — a `.cpp` compiled once elsewhere wouldn't be visible to those
call sites. This is why templates are effectively header-only by
necessity, not style. (`LogicValue.cpp` still exists, because
`logicalAnd`/`logicalOr`/`logicalNot` are *ordinary* overloaded
functions, not templates — only `LogicGate<T>` itself needs the
header-only treatment.)

## The concept: `LogicType`

```cpp
template <typename T>
concept LogicType = requires(T a, T b) {
    { logicalAnd(a, b) } -> std::same_as<T>;
    { logicalOr(a, b) } -> std::same_as<T>;
    { logicalNot(a) } -> std::same_as<T>;
};
```

`LogicGate<T>` requires `LogicType<T>`. Only `bool` and `LogicValue`
satisfy it (via the overloads in `LogicValue.h`/`.cpp`, found through
ordinary lookup). `LogicGate<int>` fails to **compile**.

**Confirmed, not just asserted** — compiled `LogicGate<int>
x(GateKind::AND);` standalone outside the project and captured the real
diagnostic:

```
error: template constraint failure for 'template<class T> requires
LogicType<T> class eda::LogicGate'
note: constraints not satisfied
...
note: 'eda::logicalAnd(a, b)' does not satisfy return-type-requirement
    { logicalAnd(a, b) } -> std::same_as<T>;
```

**More interesting than expected:** the failure isn't "no
`logicalAnd(int, int)` overload exists" — `int` implicitly converts to
`bool`, so overload resolution *finds* the `bool` overload. The concept
still correctly rejects it because that overload returns `bool`, not
`int`, failing the exact `-> std::same_as<T>` check. A real demonstration
of why concepts check more than "does some function exist" — implicit
conversions can't sneak a wrong type past `std::same_as`.

**Why this matters over the pre-C++20 alternative (SFINAE/`enable_if`):**
the error above names the exact unsatisfied requirement, at the exact
call site. The old idiom would typically surface as a wall of
substitution-failure errors from deep inside `evaluate()`'s body instead.

## Deriving NAND/NOR/XOR generically — only 3 operations needed per type

`LogicGate<T>::evaluate()` only calls `logicalAnd`/`logicalOr`/`logicalNot`
directly for `AND`/`OR`/`NOT`. `NAND`/`NOR`/`XOR` are composed from those
three *inside* `evaluate()`, generically, for any `T`:

```cpp
case GateKind::NAND: return logicalNot(logicalAnd(a, b));
case GateKind::NOR:  return logicalNot(logicalOr(a, b));
case GateKind::XOR:  return logicalOr(logicalAnd(a, logicalNot(b)),
                                       logicalAnd(logicalNot(a), b));
```

Supporting a brand new signal type later means writing exactly 3
functions (`logicalAnd`/`logicalOr`/`logicalNot` for that type), not 6 —
the same real hardware fact behind Phases 3-5's 4-NAND XOR demo circuits
(AND+OR+NOT, or NAND alone, is a "complete" logic basis) shows up again
here, now as a *code-reuse* argument, not just a circuit-design one.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_generic` (library — compiles only `LogicValue.cpp`;
`LogicGate.h` needs no separate compilation), `eda_generic_demo`
(frontend), `eda_generic_tests` (tests, 2 source files in 1 executable).

## How to run

```powershell
./build/bin/eda_generic_demo.exe
```

Prints `LogicGate<bool>` truth tables for AND/XOR (must match Phases
1/2 exactly), then `LogicGate<LogicValue>` truth tables over all 16
combinations of `{0,1,X,Z}` for the same two gates. Sample from the
`LogicValue` AND table:

```
  A  B  | OUT
  -------------
  0  0  |  0
  0  1  |  0
  0  X  |  0      <- 0 dominates: still definite
  1  X  |  X      <- genuinely unknown
```

That `0 AND X = 0` vs `1 AND X = X` distinction is the entire payoff of
this phase — it's the thing plain `bool` structurally cannot express.

## How to test

```powershell
cd build
ctest --output-on-failure
```

10 new tests, 38 total across all phases:

| Test | What it checks |
|---|---|
| `LogicValueTest.AndTruthTable` / `OrTruthTable` / `NotTruthTable` | exhaustive 4×4 (or 4) truth tables against an independently-written oracle |
| `LogicValueTest.ToCharMatchesConventionalSymbols` / `StreamOperatorMatchesToChar` | `'0'/'1'/'X'/'Z'` printing |
| `LogicGateTest.BoolInstantiationMatchesPhase1Phase2TruthTables` | `LogicGate<bool>` behaves identically to `Gate`/`Component` for all 6 gate kinds — the backward-compatibility check |
| `LogicGateTest.AndZeroDominatesEvenOverUnknown` / `OrOneDominatesEvenOverUnknown` | the dominance rules specifically |
| `LogicGateTest.XorWithUnknownInputIsUnknown` | exercises the *generically derived* XOR for a type with no XOR-specific code |
| `LogicGateTest.HighZTreatedAsUnknownForLogicNotForDominance` | `HighZ` inverts to `Unknown` but doesn't dominate AND/OR the way `Zero`/`One` do |

## What's different about this phase: no bug found

Phases 4 and 5 both found a genuine bug via testing. This phase didn't —
worth stating plainly rather than silently moving on, since the pattern
up to now might read as "every phase finds a bug." It doesn't, and
shouldn't: the risk here (a wrong concept constraint or truth table) was
caught by *reasoning through the concept/truth tables carefully before
writing code* and *then* confirming with the concept-rejection repro and
exhaustive truth-table tests — verification still happened, it just
didn't surface a defect this time. Both outcomes are "the test suite did
its job."

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues. Specific to this phase:

- **Confusing template error when misusing `LogicGate<T>`** — check the
  concept failure names `LogicType` and points at the instantiation
  site; if you see errors instead from deep inside `evaluate()`'s body,
  something's wrong with how the concept itself is declared (e.g. a typo
  breaking ADL lookup of `logicalAnd`/`logicalOr`/`logicalNot` for your
  type).
- **Adding a new signal type** — write exactly 3 free-function overloads
  (`logicalAnd`, `logicalOr`, `logicalNot`) taking and returning your
  type; `LogicGate<YourType>` works automatically once `LogicType<YourType>`
  is satisfied, no changes to `LogicGate.h` needed.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| Class template `LogicGate<T>` | `LogicGate.h` | One implementation, generic over the signal value type — `LogicGate<bool>` and `LogicGate<LogicValue>` share all their logic, differing only in which `T` the compiler instantiates. |
| Why templates are effectively header-only | `LogicGate.h`'s top comment | The compiler needs the full definition at every instantiation site; a separately-compiled `.cpp` (the pattern every previous phase used) isn't visible there. |
| C++20 concept (`concept` keyword) | `LogicGate.h` | Names a compile-time constraint (`LogicType`) once, reused as `template <LogicType T> class LogicGate`, instead of a bare unconstrained `template <typename T>`. |
| Requires-expression with return-type constraint | `LogicGate.h` | `{ logicalAnd(a, b) } -> std::same_as<T>;` — checks both "is this expression well-formed" and "does it return exactly `T`," catching the `int`-via-implicit-conversion-to-`bool` case specifically. |
| Explicit free-function overloads (not a generic primary template) | `LogicValue.h`/`.cpp` | Deliberately NO `template <typename T> T logicalAnd(T, T);` primary template — only concrete overloads for `bool`/`LogicValue`. A generic primary template would make the concept trivially "satisfied" for any `T` syntactically, even types with no real definition, defeating the point. |
| Deriving operations generically inside a template method | `LogicGate.h`, `evaluate()` | `NAND`/`NOR`/`XOR` composed from `AND`/`OR`/`NOT` inside the template itself — works for any `T` satisfying `LogicType`, without per-type code. |
| `enum class LogicValue` with non-trivial semantics | `LogicValue.h`/`.cpp` | A 4-state value where `==`/`switch` alone aren't enough to express the domain logic (dominance rules) — contrast with Phase 1's `GateType`, which really was just a tag for a `switch`. |

## Next up

**Phase 7 — Memory management:** `Netlist`/`EventSimulator`/etc. all hold
components through `std::unique_ptr` already (introduced informally in
Phase 2), but ownership rules, move semantics, and the rule-of-5 haven't
been covered rigorously yet. See `docs/ROADMAP.md` → "Why this order"
for the full chain.
