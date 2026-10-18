# Phase 2 — OOP (polymorphic gates)

**Goal:** rebuild Phase 1's six logic gates using inheritance and
polymorphism instead of one class with a `switch`, to make the tradeoff
concrete before leaning on it in later phases (Phase 3's netlist needs a
uniform way to hold many different component types).

**Status:** done. Builds clean, console output matches Phase 1's truth
tables exactly, GoogleTest suite green (7/7, plus Phase 1's 6/6 — 13/13
total).

**Isolation from Phase 1:** by explicit request, nothing from Phase 1 was
edited. Every file below is new; the only touched files are the three
`CMakeLists.txt`, and only to *append* new target blocks — verified with
`git diff --stat` showing 37 insertions, 0 deletions across all three.
`backend/include/eda/Gate.h`, `backend/src/Gate.cpp`,
`frontend/console/main.cpp`, and `tests/test_gate.cpp` are byte-for-byte
unchanged.

## Files

- `backend/include/eda/Component.h` — abstract `Component` base (pure
  virtual `evaluate()`/`name()`, virtual destructor) + six derived classes:
  `AndGate`, `OrGate`, `NotGate`, `NandGate`, `NorGate`, `XorGate`.
- `backend/src/Component.cpp` — each derived class's `evaluate()`/`name()`.
  No `switch` anywhere in this file.
- `backend/CMakeLists.txt` — new `eda_oop` static library target (appended
  below the untouched `eda_core` block).
- `frontend/console_phase2/main.cpp` + `CMakeLists.txt` — new sibling to
  `frontend/console/`. Builds a
  `std::vector<std::unique_ptr<Component>>` and prints the same truth
  tables Phase 1 does.
- `tests/test_component.cpp` — GoogleTest `TEST()` cases for all six gates
  plus one polymorphism-specific test. New `eda_component_tests` target
  appended to `tests/CMakeLists.txt`.
- `CMakeLists.txt` (top-level) — one appended line:
  `add_subdirectory(frontend/console_phase2)`.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets this phase adds: `eda_oop` (library), `eda_console_phase2`
(frontend), `eda_component_tests` (tests). Phase 1's `eda_core`,
`eda_console`, `eda_tests` are unaffected — same build, both phases
coexist.

## How to run

```powershell
./build/bin/eda_console_phase2.exe
```

Prints the same six truth tables as `eda_console.exe` (Phase 1) — the
point is that the *output* is identical; only the implementation
technique differs. Run both side by side to confirm:

```powershell
./build/bin/eda_console.exe > phase1_output.txt
./build/bin/eda_console_phase2.exe > phase2_output.txt
```
(diff them — only the banner line differs.)

## How to test

```powershell
cd build
ctest --output-on-failure
```

Now shows 13 tests: Phase 1's 6 `GateTest.*` plus Phase 2's 7
`ComponentTest.*` (`AndGate`, `OrGate`, `NotGate`, `NandGate`, `NorGate`,
`XorGate`, and `PolymorphicDispatchThroughBasePointer`). Run just this
phase's tests:

```powershell
./build/bin/eda_component_tests.exe
```

**What `PolymorphicDispatchThroughBasePointer` checks specifically:**
builds a `std::vector<std::unique_ptr<Component>>` holding one `AndGate`
and one `XorGate`, then calls `evaluate()`/`name()` through the base
`Component*` in each — confirming the *correct* override runs for each
concrete type without the caller ever naming `AndGate`/`XorGate`
directly. This is the behavior Phase 1's `Gate` has no equivalent of.

**Confirmed the suite catches real regressions:** temporarily broke
`XorGate::evaluate` (`a != b` → `a == b`). Two tests failed as a result —
not just `ComponentTest.XorGate` itself, but also
`PolymorphicDispatchThroughBasePointer` (which happens to exercise XOR
through a base pointer), showing the bug was caught both directly and
incidentally. Reverted, confirmed 13/13 green again.

## Troubleshooting

- **Only seeing Phase 1's 6 tests, not 13** — you're running stale build
  output; `cmake --build build` after pulling/adding `test_component.cpp`
  before running `ctest`.
- Everything else: see `docs/phases/phase1.md`'s troubleshooting section
  (compiler detection, stale build dir, VS Code IntelliSense, gdb) — all
  still applies, nothing toolchain-specific changed in this phase.

## What to notice when you run it

- `main.cpp` (Phase 2) has no `switch`, no `enum`, no `if`/`else` chain
  picking gate behavior anywhere. The "which gate is this" decision is
  gone from the *frontend* entirely — `printTruthTable(const Component&)`
  is written once and works for all six types, forever, including any
  future type that doesn't exist yet.
- `Component.cpp` has six tiny implementations, each knowing nothing
  about the other five. Contrast with `Gate.cpp` (Phase 1), where all six
  behaviors live in one `switch` inside one function.
- Adding a **seventh** gate type (e.g. `BufferGate`, just `return a;`) in
  Phase 2's model: write one new class, done — `main.cpp` and
  `Component.h`'s other classes need zero changes. In Phase 1's model:
  add a new `GateType` enumerator, then edit the `switch` in **both**
  `evaluate()` and `name()`. This is the concrete payoff the whole phase
  was for.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| Abstract base class | `Component.h` | A class with at least one pure virtual function can't be instantiated directly (`Component c;` is a compile error) — it exists only to be inherited from. |
| Pure virtual function (`= 0`) | `Component.h` | No body in the base; every concrete derived class *must* override it, or that class is abstract too. |
| Virtual destructor | `Component.h` (`virtual ~Component() = default;`) | Without it, `delete`ing a `Component*` that actually points at an `AndGate` would only run `~Component()`, skipping `~AndGate()` — undefined behavior once derived classes own resources. |
| Public inheritance (`: public Component`) | `Component.h` | "An `AndGate` IS-A `Component`" — callers can hold/pass it through a `Component&`/`Component*` without knowing the concrete type. |
| `override` | `Component.h` derived classes | Compiler-checked promise that this function matches a base virtual — a typo'd signature becomes a compile error instead of silently defining an unrelated new function. |
| Dynamic dispatch / vtable | `Component.cpp`, `main.cpp` | Each polymorphic object carries a hidden pointer to a table of function pointers; calling a virtual function through a base reference looks up the *actual* object's override at runtime. This is what replaces `Gate.cpp`'s `switch`. |
| `std::unique_ptr<T>` | `main.cpp`, `test_component.cpp` | Exclusive-ownership smart pointer; owns a heap-allocated object and deletes it (via the virtual destructor) automatically when the `unique_ptr` goes out of scope. Previewed here; covered in depth in Phase 7. |
| `std::make_unique<T>(...)` | `main.cpp` | Allocates a `T` and wraps it in a `unique_ptr` in one step — preferred over a hand-written `new T(...)`. |
| `std::vector<std::unique_ptr<Component>>` | `main.cpp` | A container of *different* concrete types, all accessed through one common interface — the actual payoff of polymorphism: `printTruthTable()` doesn't know or care which gate it's holding. |

## Next up

**Phase 3 — STL:** represent a netlist as a graph of `Component`s
connected by wires, using the same `Component` hierarchy introduced here
(no changes to it needed) — `std::vector`/`std::unordered_map` for
storage, topological sort for evaluation order. This is the payoff
`Component` was built for: Phase 1's `switch`-based `Gate` couldn't have
supported a heterogeneous graph this cleanly.
