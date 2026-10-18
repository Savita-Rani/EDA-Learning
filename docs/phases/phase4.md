# Phase 4 — Event-driven simulation engine

**Goal:** fix Phase 3's real limitation — `Netlist::simulate()` recomputes
*every* component on *every* call, regardless of whether anything upstream
actually changed. `EventSimulator` instead only re-evaluates components
downstream of an actual signal change, via a time-ordered event queue.

**Status:** done, after fixing one real bug found by the test suite
(not a contrived one — see below). Builds clean, demo produces correct
output, GoogleTest suite green (4/4 new, 21/21 total across all four
phases).

## Relationship to earlier phases

`EventSimulator` depends on Phase 2's `Component` (reused, not
duplicated — same pattern as Phase 3). It deliberately does **not**
depend on Phase 3's `Netlist` and is not a modification of it: this is a
second, independent technique for the *same* underlying problem (correctly
propagate values through wired-up `Component`s) — the same relationship
Phase 2 had to Phase 1 (two implementations, side by side, so the
tradeoff is visible). See `docs/DECISIONS.md`, "Phase 4 is a parallel
technique, not a Netlist extension."

Nothing in `backend/include/eda/Netlist.h`, `Netlist.cpp`,
`Component.h`, `Component.cpp`, `Gate.h`, `Gate.cpp`, or any earlier
frontend/test file was touched. The three edited `CMakeLists.txt` files
got 40 insertions, 0 deletions (verified via `git diff --stat`).

## Files

- `backend/include/eda/EventSimulator.h` / `.cpp` — `addComponent`,
  `addWatcher`, `scheduleInputChange`, `run`, `wireValue`. New
  `eda_event` static library, linking `eda_oop` `PUBLIC`.
- `frontend/event_demo/main.cpp` + `CMakeLists.txt` — the same 4-NAND
  XOR circuit as Phase 3's demo, driven by `EventSimulator`, printing
  evaluation counts to make the "only recomputes what changed" payoff
  visible.
- `tests/test_event_simulator.cpp` — 4 GoogleTest cases, new
  `eda_event_tests` target.

## A real bug, found by testing — not a contrived one

Every previous phase's "confirmed the test catches bugs" section
*injected* a bug on purpose to prove the test harness works. This phase
is different: the first version of `EventSimulator::run()` processed one
event at a time (pop an event, update its wire, immediately re-evaluate
every component reading it, push follow-up events, repeat) — and
`tests/test_event_simulator.cpp` caught a **real** correctness bug in
that design on the first run, not a planted one.

**The bug:** if two of a gate's inputs change at the exact same
simulated time (e.g. two external inputs set together at time 0), they
arrive as two *separate* events. Processing the first evaluates the gate
using the *old*, not-yet-updated value of the second input, and schedules
a result. Processing the second evaluates the gate *again*, now with the
updated second input, and schedules a *different* result — for the same
wire, at the same future time. Whichever of those two events the
priority queue happened to pop last silently overwrote the other, with no
guarantee the correct one won.

**How it was caught:** `OnlyAffectedBranchEvaluates` (two independent
`AndGate`s, no shared wires) failed with `outB` changing value even
though nothing touched its inputs — and `XorCircuitSettlesToCorrectValue`
failed specifically for `XOR(true, true)`, both exactly where this race
would bite (two inputs changing simultaneously). Diagnosed with a
throwaway instrumented repro (watchers printing every transition) outside
the test suite, which showed `outB` flipping `true` → `false` with no
input change responsible for the second flip.

**The fix:** batch every event scheduled for the same time instant
together before evaluating anything. Apply all their wire-value changes
first, *then* decide which components are actually affected (deduplicated
— a gate with two inputs changing in the same instant is evaluated
exactly once, with both new values visible), *then* schedule next-time-step
events from that fully-settled snapshot. This is "delta cycle" batching —
the standard technique real event-driven HDL simulators use for exactly
this reason. See the comment block at the top of `EventSimulator.cpp` for
the full explanation kept with the code.

After the fix: all 4 tests pass, including both that caught the bug.

## What this phase doesn't handle (a real, known limitation)

Wires default to `false` when first created (`addComponent` pre-seeds
each output wire's value). If a circuit's correct settled value for some
wire *happens* to equal that default, `EventSimulator` can't tell "this
was computed and really is false" apart from "this was never actually
set" — no transition gets recorded, so no watcher fires for it. This
isn't a bug to fix here: it's the standard reason real digital
simulators use a third "unknown" (`X`) state for uninitialized signals
instead of defaulting to a real logic value. That's exactly what
**Phase 6** (generic/multi-valued signals, 0/1/X/Z) is for — this
limitation is a concrete preview of why that phase exists, not just an
abstract "templates are nice" justification.

(One test — `WatcherFiresOnlyOnActualTransition` — initially tripped over
this directly: it used `A=false, B=false`, whose correct XOR is `false`,
the same as the default, so the watcher legitimately never fired. Fixed
by using `A=false, B=true` instead, whose correct XOR (`true`) differs
from the default and reliably produces an observable transition.)

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_event` (library), `eda_event_demo` (frontend),
`eda_event_tests` (tests). All prior targets (Phases 1-3) build
unchanged alongside these.

## How to run

```powershell
./build/bin/eda_event_demo.exe
```

```
=== EDA Logic Simulator — Phase 4: Event-driven engine ===
Same 4-NAND XOR circuit as Phase 3, driven by events instead of a full recompute.

-- Initial inputs: A=0, B=0 --
OUT = 0  (6 component evaluations)

-- Flip B: 0 -> 1 --
  [watcher] OUT changed to 1
OUT = 1  (3 component evaluations)

-- Set B to the SAME value again (1 -> 1, no real change) --
OUT = 1  (0 component evaluations — nothing changed, nothing ran)
```

Contrast with Phase 3: `Netlist::simulate()` evaluates all 4 gates on
*every* call, no matter what. Here, flipping one input evaluates only 3
(not all 4 — `n1` plus the two gates downstream of it), and re-asserting
an already-current value evaluates **zero**.

## How to test

```powershell
cd build
ctest --output-on-failure
```

4 new tests (`EventSimulatorTest.*`), 21 total across all phases:

| Test | What it checks |
|---|---|
| `XorCircuitSettlesToCorrectValue` | all 4 input combinations settle to the correct XOR value — this is the one that caught the original bug for `(true, true)` |
| `NoEvaluationsWhenInputDoesNotActuallyChange` | re-asserting an input's current value produces `run() == 0` evaluations |
| `WatcherFiresOnlyOnActualTransition` | the Observer callback fires exactly once per real transition, not once per event |
| `OnlyAffectedBranchEvaluates` | two independent `AndGate`s, no shared wires — flipping one branch's input evaluates exactly 1 component and leaves the other branch's output completely untouched; this is the one that caught the original bug's cross-branch symptom |

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section — nothing new
toolchain-wise this phase.

- **A watcher doesn't fire when you expected it to** — check whether the
  wire's correct value happens to equal its `false` default (see "What
  this phase doesn't handle" above) before assuming it's a bug.

## What to notice when you run it

- The evaluation counts printed by the demo are the entire point of this
  phase. Phase 3's `eda_netlist_demo.exe` has no equivalent number to
  print — it always does the same fixed amount of work.
- `OnlyAffectedBranchEvaluates` is the cleanest possible demonstration:
  two gates that share no wires at all, and changing one branch's input
  provably never touches the other.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `std::priority_queue` with a custom comparator | `EventSimulator.h` | Max-heap by default; `EventComparator` inverts the comparison to get min-heap-by-time behavior — smallest scheduled time processed first. |
| `std::function<void(bool)>` | `EventSimulator.h`/`.cpp` | The Observer pattern's callback type — a watcher is "anything callable with a `bool`," whether a lambda, free function, or bound member function. |
| Lambdas as callbacks | `frontend/event_demo/main.cpp`, tests | How calling code actually registers a watcher — `sim.addWatcher("OUT", [](bool v){ ... });`. |
| Observer pattern | `EventSimulator.cpp` | Wires notify registered watchers on an actual value change, decoupled from the internal fanout-based propagation that drives the simulation itself — two separate notification paths serving two separate purposes. |
| `std::unordered_set<std::size_t>` for deduplication | `EventSimulator.cpp`, `run()` | The actual fix for the race condition above: ensures a component with multiple changed inputs in one instant is evaluated once, not once per changed input. |
| "Delta cycle" batching | `EventSimulator.cpp`, `run()` | Standard event-driven-simulator technique: fully apply all same-instant changes before evaluating anything that depends on them. |
| Glitches | demo output, `EventSimulator.cpp` comment | A signal can pass through an intermediate value before settling to its final one — a real phenomenon in real digital circuits, not a simulation artifact to suppress. |

## Next up

**Phase 5 — File I/O:** circuits are still hand-built in C++ code
(`addComponent` calls) — no way to describe a circuit as data or load one
from a file. Phase 5 fixes that with a netlist file format and a parser.
See `docs/ROADMAP.md` → "Why this order" for the full causal chain.
