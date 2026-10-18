# Phase 3 — Netlist (STL)

**Goal:** represent a circuit as a graph of `Component`s connected by
named wires, and evaluate it automatically in the correct order via
topological sort — replacing "manually call `evaluate()` on one gate" with
"describe a circuit, then simulate it."

**Status:** done. Builds clean, 4-NAND XOR demo produces the correct
truth table, GoogleTest suite green (4/4 new, 17/17 total across all
three phases).

## Relationship to earlier phases (read this first)

Unlike Phase 2 (a deliberate *parallel reimplementation* of Phase 1's
`Gate`, built for side-by-side comparison), Phase 3 **depends on** Phase
2's `Component` — `Netlist` stores `std::unique_ptr<Component>`, same
interface, unchanged. This was the plan from Phase 2's own "Next up" note
and from `docs/ROADMAP.md`'s causal-chain table: Phase 2 built the
uniform interface specifically so Phase 3 could hold a mix of different
gate types in one graph. See `docs/DECISIONS.md`, "Phase 3 depends on
Phase 2" for why this isn't a violation of the phase-isolation rule —
*depending on* a previous phase's library (new code, nothing edited) is
different from *editing* it.

Still true: nothing in `backend/include/eda/Component.h`,
`backend/src/Component.cpp`, `backend/include/eda/Gate.h`,
`backend/src/Gate.cpp`, or any earlier frontend/test file was touched.
`git diff --stat` on the three edited `CMakeLists.txt` files showed 39
insertions, 0 deletions.

## Files

- `backend/include/eda/Netlist.h` / `backend/src/Netlist.cpp` — the
  `Netlist` class: `addComponent`, `setInput`, `evaluationOrder`,
  `simulate`, `wireValue`. New `eda_netlist` static library, linking
  `eda_oop` `PUBLIC` (so anything linking `eda_netlist` transitively gets
  `Component.h` too).
- `frontend/netlist_demo/main.cpp` + `CMakeLists.txt` — builds a 4-NAND
  decomposition of XOR and simulates it for all 4 input combinations.
- `tests/test_netlist.cpp` — 4 GoogleTest cases, new `eda_netlist_tests`
  target.

## Why there's no separate `Wire`/`Signal` class

The roadmap originally listed "Wire/Signal" as a concept for this phase.
Implemented instead as `std::unordered_map<std::string, bool>` (wire name
→ current value) inside `Netlist`. A dedicated `Wire`/`Signal` class would
carry a name and a value and no behavior beyond that — pure ceremony here.
Named wires *are* the signal concept; the map is just a lighter-weight
realization of it. If a future phase needs richer per-wire state (e.g.
Phase 6's multi-valued 0/1/X/Z signals), that's the natural point to
introduce a real `Signal` type — not before it's needed.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_netlist` (library), `eda_netlist_demo` (frontend),
`eda_netlist_tests` (tests). All prior targets (Phases 1 and 2) still
build unchanged alongside these.

## How to run

```powershell
./build/bin/eda_netlist_demo.exe
```

```
=== EDA Logic Simulator — Phase 3: Netlist (STL) ===
4-NAND XOR circuit: n1=NAND(A,B) n2=NAND(A,n1) n3=NAND(B,n1) OUT=NAND(n2,n3)

  A  B  | OUT
  -------------
  0  0  |  0
  0  1  |  1
  1  0  |  1
  1  1  |  0
```

That's XOR's truth table, produced by four NAND gates wired together and
simulated — not a direct XOR gate call. `n1` must be computed before `n2`
and `n3` (both read it), and both of those before `OUT` — `simulate()`
works this out itself via `evaluationOrder()`.

## How to test

```powershell
cd build
ctest --output-on-failure
```

4 new tests (`NetlistTest.*`), 17 total across all phases:

| Test | What it checks |
|---|---|
| `SingleAndGate` | basic sanity: one component, two inputs, correct output |
| `UnknownWireDefaultsFalse` | reading a never-set/never-produced wire returns `false`, not garbage |
| `SimulatesXorRegardlessOfInsertionOrder` | **the real test** — adds the 4 NAND gates in *reverse* dependency order (the entry for `OUT` is added first, `n1` last) and checks `simulate()` still produces correct XOR for all 4 input combinations |
| `ThrowsOnFeedbackLoop` | a 2-gate cycle (`X` depends on `Y`, `Y` depends on `X`) makes `simulate()` throw `std::runtime_error`, via `EXPECT_THROW` |

**Why `SimulatesXorRegardlessOfInsertionOrder` is the one that matters:**
if `Netlist::simulate()` evaluated components in the order they were
*added* rather than computing a real topological order, this specific
test would fail — `n2`/`n3` would read `n1` before it's been computed
(stale `false` default), and `OUT` would read stale `n2`/`n3`. Confirmed
this directly: temporarily made `simulate()` iterate entries in plain
insertion order instead of calling `evaluationOrder()`, rebuilt, and
watched it fail —

```
Expected equality of these values:
  netlist.wireValue("OUT")
    Which is: true
  expectedXor(a, b)
    Which is: false
XOR(true, true)
[  FAILED  ] NetlistTest.SimulatesXorRegardlessOfInsertionOrder
```

— and `ThrowsOnFeedbackLoop` failed too in that state (bypassing
`evaluationOrder()` also bypasses the cycle check it does). Reverted,
confirmed 17/17 green again.

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section — compiler
detection, stale build dir, VS Code IntelliSense, gdb — all still
applies. Nothing new toolchain-wise this phase.

## What to notice when you run it

- `main.cpp` never calls `NandGate::evaluate()` directly, and never
  decides what order to evaluate the 4 gates in — it only describes the
  wiring (`addComponent(..., {"A","B"}, "n1")` etc.) and calls
  `simulate()`. The ordering logic is entirely inside `Netlist`.
- The demo deliberately adds components in a *readable* order (n1, n2,
  n3, OUT — matching how you'd naturally describe the circuit on paper).
  The test file proves this isn't load-bearing: adding them in the exact
  opposite order still produces correct results, because
  `evaluationOrder()` doesn't care what order you called
  `addComponent()` in.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `std::unordered_map<std::string, bool>` | `Netlist.h`/`.cpp` | Wire name → value lookup; O(1) average access instead of scanning a list every time a wire's value is needed. |
| `std::vector<Entry>` with a private nested struct | `Netlist.h` | Groups "one component + its wiring" without needing a type visible outside `Netlist` — `Entry` has no reason to exist independently. |
| Pass-by-value + `std::move` vs pass-by-`const&` | `addComponent` | `component` (a `unique_ptr`, ownership transferred) is taken by value; `inputWires`/`outputWire` (read, not owned) by `const&` — the signature documents intent before you read the body. |
| Topological sort (Kahn's algorithm) | `evaluationOrder()` | Classic graph algorithm: in-degree counts + a queue of "ready" nodes, built entirely from `std::vector`/`std::unordered_map`/`std::queue`. Directly answers "what order must these run in." |
| `std::queue` | `evaluationOrder()` | FIFO structure driving Kahn's algorithm — "ready" entries are processed in the order they became ready. |
| Exceptions (`std::runtime_error`, `EXPECT_THROW`) | `Netlist.cpp`, `test_netlist.cpp` | A netlist with a feedback loop has no valid evaluation order — that's a real error condition, not a case to silently return wrong data for. First real use of exceptions in this project (Phase 5 covers them more deeply for file parsing). |
| Transitive `PUBLIC` linking | `backend/CMakeLists.txt` | `eda_netlist` links `eda_oop` `PUBLIC` — anything linking `eda_netlist` automatically gets `eda_oop` too, without saying so explicitly. |

## Next up

**Phase 4 — Event-driven simulation engine:** right now, `simulate()`
recomputes *every* component from scratch on every call, even if only one
input changed. An event-driven scheduler only recomputes what actually
needs it when a signal changes — using lambdas/`std::function` as
callbacks and a `priority_queue` for scheduling. This fixes what Phase 3
leaves open: there's still no notion of "time" or "only update what
changed" — see `docs/ROADMAP.md` → "Why this order" for the full chain.
