# Phase 12 — Performance

**Goal:** Phase 11 made correctness trustworthy (measured coverage,
sanitizers, CI). Nothing before this phase had been measured for
*speed* — optimizing earlier would have been wasted effort on code that
was still changing shape every phase. This phase profiles a real
bottleneck with actual measurements, fixes it in a new, independent
implementation (never by editing the original), and reports honest
numbers — not an assumed "this should be faster."

**Status:** done. Found a real, measured inefficiency in Phase 3's
`Netlist`, confirmed it with a wall-clock benchmark (not a guess),
fixed it in a brand-new `FastNetlist` (not an edit to `Netlist`), and
measured a consistent **~18.5x** speedup across three different circuit
sizes. `gprof` was tried first for CPU profiling and found to be
genuinely broken on this specific toolchain (confirmed directly, not
assumed) — documented honestly below rather than silently worked
around.

## The bottleneck: found by reading the code, confirmed by measuring it

Looking at `Netlist::simulate()` (`backend/src/Netlist.cpp`):

```cpp
void Netlist::simulate() {
    for (std::size_t idx : evaluationOrder()) {   // <-- recomputed EVERY call
        ...
    }
}
```

`evaluationOrder()` runs the full Kahn's-algorithm topological sort —
building a `producerOf` hash map, a `dependents` adjacency list, and an
`inDegree` vector, all from scratch — **every single time `simulate()`
is called**, even though the circuit's *topology* (which component
produces which wire) never changes between calls; only wire *values*
do. A clocked simulation loop (set inputs, `simulate()`, read outputs,
repeat) pays this rebuild cost on every tick, for no reason.

A second, smaller cost stacks on top: every wire read/write
(`wireValue()`, `wireValues_[...]`) hashes a `std::string` via
`std::unordered_map<std::string, bool>` — once per input per component,
every call.

Neither of these is a complexity-class problem — `evaluationOrder()`
and `simulate()`'s evaluation loop are both `O(n)` in the number of
components, same as any correct implementation would be. The real cost
is in the **constant factor**: `Netlist` repeats an expensive
`O(n)` rebuild step (hash-map construction, vector-of-vectors
allocation, string hashing) that provides *no new information* on
repeat calls, when it could just remember the answer.

## Profiling: `gprof` tried first, found genuinely broken on this toolchain

Before assuming the code-reading diagnosis above was the real
bottleneck, tried to confirm it with `gprof` — same "measure, don't
guess" standard this project held itself to for sanitizers and
coverage in Phase 11.

Added `EDA_ENABLE_PROFILING` (`-pg`, GCC/Clang's built-in profiling
instrumentation, same opt-in pattern as `EDA_ENABLE_COVERAGE`/
`EDA_ENABLE_SANITIZERS`), built `eda_netlist_bench` with it, ran it,
and profiled the resulting `gmon.out`:

```
$ gprof eda_netlist_bench.exe gmon.out
Flat profile:
  (no rows — empty)
Call graph: (no rows — empty)
```

**Checked why, didn't just shrug:**

```
$ objdump -r Netlist.cpp.obj | grep -i mcount
(nothing)
$ objdump -r Netlist.cpp.obj
...
0000000000000001 IMAGE_REL_AMD64_REL32  __fentry__
...
```

This GCC (13.2.0, MinGW/ucrt64) instruments `-pg` builds with calls to
`__fentry__`, not the classic `mcount` symbol `gprof` expects on this
target — confirmed with a minimal 2-function standalone repro first
(same methodology as Phase 11's ASan repro), which showed the identical
`__fentry__` relocation. Tried the documented escape hatch:

```
$ g++ -pg -mno-fentry -O2 test.cpp -o test.exe
cc1plus.exe: sorry, unimplemented: '-mno-fentry' isn't compatible with SEH
```

`-mno-fentry` is flatly rejected — x86-64 Windows requires SEH
(structured exception handling), and SEH requires `-mfentry`'s
prologue hooks here, so there's no way to get classic `mcount`-style
instrumentation on this toolchain at all. A real, confirmed platform
limitation, in the same spirit as Phase 11's ASan/UBSan finding on
this same toolchain — not a mistake in how `-pg` was invoked.

**What this phase did instead:** fell back to direct wall-clock
measurement (`std::chrono::steady_clock`, see below) instead of
sampling/instrumentation profiling. Less granular (no per-function
breakdown), but the code-reading diagnosis above is specific and
testable: if the rebuild-every-call cost is real, a benchmark holding
circuit size fixed and varying call count should show the cost scaling
with calls, and `FastNetlist`'s fix (cache the rebuild) should show a
*consistent* speedup regardless of circuit size, not one that changes
with scale (a real complexity-class win would show a growing or
shrinking ratio as size increases; a real constant-factor win holds
the ratio steady). Both predictions were checked against real numbers
below, not just asserted.

`EDA_ENABLE_PROFILING` is kept in the build either way — it's still a
real GCC feature, just not useful for a flat/call-graph breakdown via
`gprof` on *this* target; a toolchain where `gprof` does understand
`__fentry__` (or classic `mcount`, e.g. non-Windows GCC targets) could
use it unchanged.

## FastNetlist: a new, independent implementation — not an edit to Netlist

`backend/include/eda/FastNetlist.h` + `backend/src/FastNetlist.cpp` — a
second implementation of the same problem `Netlist` solves, same
public shape (`addComponent`/`setInput`/`simulate`/`wireValue`), built
alongside it rather than by editing it. This project's isolation rule
(never edit a previous phase's files) already has exactly this shape
of exception handled: Phase 4's `EventSimulator` and Phase 6's
`LogicGate<T>` are both independent, parallel techniques for a problem
an earlier phase already solved, kept separate specifically so the two
can be compared. See `docs/DECISIONS.md`, "Phase 12 is a parallel
technique, not a Netlist edit."

Two changes, both targeting the two costs found above:

1. **Cached evaluation order.** A `bool evaluationOrderValid_` flag.
   `simulate()` only calls `rebuildEvaluationOrder()` when the flag is
   false; `addComponent()` is the only thing that sets it false (adding
   a component is the only way the topology can change). Every
   `simulate()` call after the first on an unchanged netlist reuses the
   cached order directly.
2. **Interned wire indices instead of string keys.** Wire names are
   resolved to small integers once, the first time `addComponent()`/
   `setInput()` sees them (`internWire()`), and stored in a
   `std::vector<bool>` instead of `std::unordered_map<std::string,
   bool>`. Every `simulate()` call reads/writes that vector by index —
   no string hashing anywhere in the hot loop.

## Correctness first — verified before trusting any benchmark comparison

A benchmark comparing a correct implementation against a buggy one
proves nothing. `tests/test_fast_netlist.cpp` (new `eda_fast_netlist_tests`
target) mirrors every case from Phase 3's own `test_netlist.cpp` —
including the worst-case insertion-order XOR test and the feedback-loop
exception test — plus two cases specific to the caching logic itself:

- `CachedEvaluationOrderInvalidatedByLaterAddComponent`: simulates once
  (building and caching a 1-entry order), **then** adds a second
  component, **then** simulates again. If `addComponent()`'s
  `evaluationOrderValid_ = false` were missing, this would silently
  reuse the stale 1-entry order and never evaluate the new component —
  exactly the kind of bug a cache invalidation path can hide.
- `RepeatedSimulateWithChangingInputsStaysCorrect`: five `simulate()`
  calls with different inputs each time, same cached order reused
  across all five, checked against the expected XOR truth table every
  iteration.

All 6 pass. Full suite (96 tests — the 90 from Phase 11 plus these 6):

```
100% tests passed, 0 tests failed out of 96
```

## The benchmark — real numbers, three circuit sizes

`benchmarks/netlist_bench.cpp` (new `eda_netlist_bench` target, no
GoogleTest/GLFW dependency — builds under both the primary MinGW
toolchain and MSVC): builds a dependency **chain** of `NotGate`s
(`w0 -> NOT -> w1 -> NOT -> w2 -> ...`) — the worst case for `Netlist`'s
rebuild, since every entry depends on exactly the previous one — then
calls `simulate()` repeatedly on the same instance, timed with
`std::chrono::steady_clock`.

| Chain length | `simulate()` calls | `Netlist` (Phase 3) | `FastNetlist` (Phase 12) | Speedup |
|---|---|---|---|---|
| 50    | 20,000 | 1,274.98 ms  | 69.21 ms  | **18.42x** |
| 500   | 20,000 | 12,759.10 ms | 682.32 ms | **18.70x** |
| 2,000 | 2,000  | 5,069.08 ms  | 273.03 ms | **18.57x** |

**The speedup ratio holds steady (~18.5x) across a 40x range in
circuit size** — exactly what a constant-factor fix predicts, and
exactly NOT what a complexity-class fix would show (that would make the
ratio grow or shrink with size). This confirms the code-reading
diagnosis directly from measured data, filling in for the `gprof`
breakdown that wasn't available on this toolchain.

Per-call cost, computed from the same data (total time / call count),
also lines up with the "both are `O(n)` per call" claim — `Netlist`'s
per-call cost at chain length 500 (0.638 ms) is almost exactly 10x its
cost at chain length 50 (0.0637 ms), matching the 10x size increase:

| Chain length | `Netlist` ms/call | `FastNetlist` ms/call |
|---|---|---|
| 50    | 0.0637 | 0.00346 |
| 500   | 0.638  | 0.0341  |
| 2,000 | 2.535  | 0.137   |

## Relationship to earlier phases

`FastNetlist` depends on `eda_oop`'s `Component` (reused, like Phases
3/4/6/8/9/10) but is independent of `eda_netlist` — a parallel
technique, not an extension. `benchmarks/eda_netlist_bench` links both
`eda_netlist` and `eda_fast_netlist` directly to compare them, same way
any demo links multiple backend libraries. `Netlist.cpp` was not
touched — confirmed by `git diff` on this phase showing no changes to
any Phase 3 file.

## Files

- `backend/include/eda/FastNetlist.h` + `backend/src/FastNetlist.cpp` —
  the optimized, independent implementation.
- `backend/CMakeLists.txt` — new `eda_fast_netlist` library (appended).
- `tests/test_fast_netlist.cpp` + `tests/CMakeLists.txt` append — new
  `eda_fast_netlist_tests` target, 6 tests.
- `benchmarks/CMakeLists.txt` + `benchmarks/netlist_bench.cpp` — new
  top-level `benchmarks/` directory (sibling to `backend/`/`frontend/`/
  `tests/`), new `eda_netlist_bench` target.
- `CMakeLists.txt` — `EDA_ENABLE_PROFILING` option (appended),
  `add_subdirectory(benchmarks)` (appended).
- `.gitignore` — added `build-profile/`, `gmon.out`.

## How to build and run

```powershell
# Normal build/test (unaffected — new options default OFF):
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build; ctest --output-on-failure   # 96 tests

# Run the benchmark directly (chainLength, iterations — both optional):
./build/bin/eda_netlist_bench.exe 500 20000

# Profiling build (gprof — confirmed broken on this toolchain, see above;
# kept for toolchains where it isn't):
cmake -S . -B build-profile -G "MinGW Makefiles" -DEDA_ENABLE_PROFILING=ON
cmake --build build-profile --target eda_netlist_bench
cd build-profile/bin; ./eda_netlist_bench.exe; gprof eda_netlist_bench.exe gmon.out
```

## How to test

```powershell
cd build
ctest --output-on-failure
```

6 new tests, 96 total across all phases.

| Test | What it checks |
|---|---|
| `FastNetlistTest.SingleAndGate` | basic correctness, mirrors `NetlistTest.SingleAndGate` |
| `FastNetlistTest.UnknownWireDefaultsFalse` | mirrors `NetlistTest.UnknownWireDefaultsFalse` |
| `FastNetlistTest.SimulatesXorRegardlessOfInsertionOrder` | worst-case insertion order still topologically sorts correctly, mirrors `NetlistTest`'s equivalent |
| `FastNetlistTest.ThrowsOnFeedbackLoop` | mirrors `NetlistTest.ThrowsOnFeedbackLoop` |
| `FastNetlistTest.CachedEvaluationOrderInvalidatedByLaterAddComponent` | the one bug class unique to caching: stale order after a topology change |
| `FastNetlistTest.RepeatedSimulateWithChangingInputsStaysCorrect` | reusing the same cached order across multiple calls stays correct as inputs change |

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues.

- **`eda_netlist_bench` not found after adding `benchmarks/`** — run
  `cmake -S . -B build` again (adding a new `add_subdirectory` requires
  reconfiguring, same as any new CMake target).
- **`gprof` reports an empty flat profile / call graph** — expected on
  this toolchain (GCC 13.2.0, MinGW/ucrt64's `-mfentry`-based `-pg`
  instrumentation + SEH requirement; see above). Not a sign the build
  is broken — `ctest` and the benchmark both still work normally. Use
  the wall-clock benchmark instead, or try a non-Windows GCC target if
  a real `gprof` breakdown is needed.
- **Benchmark numbers vary noticeably between runs** — expected; this
  is a simple wall-clock measurement on a general-purpose OS, not a
  dedicated benchmarking harness with warm-up/statistical rigor. The
  ~18.5x ratio was stable across three very different circuit sizes,
  which is the claim being made — not that any single run's absolute
  millisecond figure is reproducible to the decimal.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| Constant-factor vs. complexity-class optimization | the benchmark table — same ~18.5x ratio across a 40x size range | A real, common distinction: `Netlist`'s problem was never its Big-O, it was *redoing* `O(n)` work that didn't need redoing. Recognizing which one you're looking at changes what "fixing" the code means. |
| Lazy caching with an explicit dirty flag | `FastNetlist::evaluationOrderValid_` | A general pattern — recompute only when the thing you depend on (topology) actually changed, not on every use. |
| Interning strings to integer indices | `FastNetlist::internWire()` / `wireIndex_` | A classic technique anywhere a hot loop would otherwise re-hash the same small set of string keys repeatedly — trade a one-time lookup for array-speed access afterward. |
| Profiling tooling can be broken by the platform itself | the `gprof`/`__fentry__`/SEH investigation | `-pg` "should" work the same everywhere; it didn't here, for a real, checkable reason (an ABI requirement of this specific OS/architecture target), not a flag typo. Confirming *why* before working around it is what made the workaround (wall-clock benchmarking) actually trustworthy. |
| `std::chrono::steady_clock` for wall-clock benchmarking | `benchmarks/netlist_bench.cpp` | `steady_clock`, not `system_clock` — monotonic, unaffected by wall-clock adjustments, the correct choice for measuring elapsed durations. |

## Next up

This was the last phase on the original roadmap
(`docs/ROADMAP.md`) — Phases 1 through 12 now cover basics through
performance. Any further work from here is additional depth on an
existing area (e.g. a Qt frontend, more extensive benchmarking) rather
than a new rung on the original ladder.
