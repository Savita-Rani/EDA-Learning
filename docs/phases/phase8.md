# Phase 8 — Concurrency

**Goal:** fix Phase 7's leftover limitation — simulating a netlist is
still single-threaded; independent parts of a circuit evaluate serially
even though nothing requires that. `ParallelSimulator` evaluates every
dependency *level* of a circuit concurrently via a hand-written
`ThreadPool`, with correctness guaranteed by careful, explicit
synchronization (`std::mutex`, `std::atomic`) rather than hoped for.

**Status:** done. Builds clean, correctness demo matches Phases 3/4/5,
speedup demo shows a real, honestly-labeled ~9× speedup on artificially
expensive gates, GoogleTest suite green (11/11 new, 66/66 total across
all eight phases) — including 20 repeated runs of both new test
binaries with zero flakiness, since concurrency bugs are notoriously
intermittent and a single clean pass doesn't rule that out. No bug
found in the final design (see "A bug avoided by design," below, for
one that *would* have appeared here had this been built less
carefully).

## Relationship to earlier phases

Two pieces, two different relationships:

- **`ThreadPool`** — fully **independent**, generic infrastructure (no
  dependency on `Component`/`Gate`/etc.), same category as Phase 7's
  `OwningPtr`/`SharedOwningPtr`/`FileHandle`. A thread pool doesn't know
  or care what kind of task it runs.
- **`ParallelSimulator`** — **depends on** both `ThreadPool` (this same
  phase) and Phase 2's `Component` (reused, like Phases 3/4/6) — new
  capability built on two existing pieces.

**Not** a parallel-technique-vs-`Netlist` situation the way Phase 4 was
to Phase 3, even though `ParallelSimulator` and `Netlist` both compute
"every wire's value." The axis of improvement is different: Phase 4
made the engine do *less* work (incremental — only recompute what
changed). Phase 8 does the exact *same* amount of work as `Netlist`
(full recompute, every `simulate()` call) but runs independent pieces
of it *concurrently* instead of serially. Two genuinely different
optimizations, not a rematch of the same one.

Nothing in any earlier phase's files was touched. The three edited
`CMakeLists.txt` got 72 insertions, 0 deletions.

## Files

- `backend/include/eda/ThreadPool.h` / `.cpp` — fixed worker threads,
  one shared task queue, `submit()`/`waitUntilIdle()`.
- `backend/include/eda/ParallelSimulator.h` / `.cpp` — level-based
  parallel evaluation: `addComponent`/`setInput`/`simulate`/`wireValue`/
  `evaluationCount`.
- `frontend/parallel_demo/main.cpp` + `CMakeLists.txt` — correctness
  check (4-NAND XOR) plus an honestly-labeled artificial speedup
  demonstration.
- `tests/test_thread_pool.cpp` (5 tests) + `test_parallel_simulator.cpp`
  (6 tests) — two separate `eda_threadpool_tests`/`eda_parallel_tests`
  targets, matching the two-library split.

## How independent work is found: levels, not just an order

`Netlist`'s `evaluationOrder()` (Phase 3) produces a single flat order —
useful for sequential execution, useless for parallelism (it doesn't
say *which* entries could safely run at the same time). `ParallelSimulator::
computeLevels()` runs the same Kahn's-algorithm idea but groups entries
into **levels** instead: level 0 has no dependencies at all; level *k*'s
entries depend only on levels `< k`. Every entry *within* one level is
independent of every other entry in that level by construction, so
they can all be submitted to the thread pool at once; levels themselves
must still run in strict order — one `ThreadPool::waitUntilIdle()`
barrier between each.

## A bug avoided by design (not found — prevented)

Every earlier concurrency-adjacent design decision in this project
(see `ThreadPool.cpp`'s top comment) was made specifically to avoid a
well-known "lost wakeup" race: if a worker thread decremented
`pendingTasks_` and called `notify_all()` **without** holding the same
mutex `waitUntilIdle()` uses to check its predicate, there's a window
where the notification could fire *before* the waiting thread actually
starts waiting — and since `condition_variable::notify_all()` doesn't
queue up missed notifications, that wakeup is lost forever, hanging
`waitUntilIdle()` permanently.

This project's `ThreadPool` keeps the decrement, the zero-check, *and*
the notify all under one lock — closing the gap entirely. Verified this
wasn't just theoretical caution: ran both new test binaries 20
consecutive times each (40 total runs) specifically looking for
intermittent hangs or wrong counts, since a *subtly* wrong
synchronization design often passes most of the time and only fails
occasionally under different thread-scheduling timing. Zero failures.

(Contrast with Phases 4 and 5, which found *and fixed* real bugs after
the fact. This phase's carefulness happened *before* the first build —
worth being honest that "no bug found" sometimes means "found during
design," not "got lucky.")

## The speedup demo, and why it's artificial — said plainly

This project's actual gates (`AndGate::evaluate()` etc.) are a handful
of boolean operations — nanoseconds. Threading overhead (locking,
scheduling, context switches) would dominate completely at that scale;
multithreading 4-6 trivial boolean ops would almost certainly be
*slower* than doing them serially, not faster. `ParallelSimulator`
accepts an optional `simulatedGateCost` specifically to make the
*mechanism* demonstrable — defaults to zero (real speed, used by every
test), and the demo sets it to 20ms per gate, stated explicitly as
artificial, standing in for "imagine this gate's evaluation were
genuinely expensive (e.g. a complex multi-bit arithmetic unit, or a
call out to an external model)."

```
8 independent gates, 1 worker thread:
  elapsed: 247.78 ms
8 independent gates, 8 worker threads:
  elapsed: 27.51 ms
speedup: 9.0x
```

(Slightly *better* than the theoretical 8× — timing noise/OS scheduling
variance on this run, not a claim of superlinear speedup; rerun and the
exact number moves around, though it stays in the same ballpark.)

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_threadpool`, `eda_parallel` (libraries),
`eda_parallel_demo` (frontend), `eda_threadpool_tests`,
`eda_parallel_tests` (tests). CMake's `find_package(Threads REQUIRED)`
locates the platform thread library automatically — confirmed working
on this project's MinGW/ucrt64 toolchain without any extra flags.

## How to run

```powershell
./build/bin/eda_parallel_demo.exe
```

Takes noticeably longer than other phases' demos to finish (~275ms) —
that's the artificial gate cost deliberately making the timing
difference visible, not a performance regression.

## How to test

```powershell
cd build
ctest --output-on-failure
```

11 new tests, 66 total across all phases. No timing-based assertions in
the test suite at all — deliberately, since asserting "X was faster
than Y" in a test is a classic source of flaky CI on a loaded machine.
Tests check pure correctness instead.

| Test | What it checks |
|---|---|
| `ThreadPoolTest.AllSubmittedTasksRun` | every submitted task actually executes exactly once |
| `ThreadPoolTest.DestructorDrainsRemainingQueuedTasksBeforeStopping` | letting a pool go out of scope with queued-but-not-yet-run tasks still finishes them, rather than abandoning them |
| `ThreadPoolTest.MultipleRoundsOfSubmitAndWaitAreIndependentBarriers` | the pool is reusable across repeated submit/wait cycles — exactly how `ParallelSimulator` uses it, once per level |
| `ParallelSimulatorTest.IndependentBranchesBothCorrectWhenRunConcurrently` | two gates sharing no wires, evaluated concurrently, both land on the correct value |
| `ParallelSimulatorTest.EvaluationCountReflectsOnlyTheMostRecentSimulateCall` | catches exactly the reset bug described below |
| `ParallelSimulatorTest.ManyIndependentGatesAllCorrectUnderConcurrency` | a 30-gate stress test — more concurrent entries gives a real race more opportunity to surface than 1-2 would |

**One real mismatch caught while writing this phase (fixed before any
test ran against it, but worth recording honestly):** the header
documented `evaluationCount()` as "how many evaluations the most
recent `simulate()` call performed," but the implementation never
reset the counter between calls — it would have accumulated across
every `simulate()` call forever, contradicting its own documented
contract. Caught by re-reading the header against the implementation
side by side before building, not by a failing test (the test
`EvaluationCountReflectsOnlyTheMostRecentSimulateCall` was written
*after* to guard the fix). Fixed with one line:
`evaluationCount_.store(0, ...)` at the top of `simulate()`.

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues. Specific to this phase:

- **A test using `ThreadPool`/`ParallelSimulator` seems to hang** —
  this would most likely indicate a reintroduced lost-wakeup bug (see
  above). Check that every modification of `pendingTasks_` and every
  `notify_*` call tied to it happens under the same `mutex_`.
- **`find_package(Threads REQUIRED)` fails** — shouldn't happen on a
  standard MinGW/MSYS2 or Linux/macOS toolchain; if it does, confirm
  the compiler supports `<thread>` at all (`g++ --version`, needs a
  reasonably modern libstdc++).

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `std::thread` | `ThreadPool` constructor/destructor | One OS thread per worker, started in the constructor, joined in the destructor — RAII applied to threads, same promise as Phase 7's classes. |
| `std::mutex` + `std::condition_variable` | `ThreadPool.cpp` | The standard producer/consumer pattern; the specific "lock on both sides" discipline that avoids the lost-wakeup race. |
| `std::atomic<std::size_t>` | `ParallelSimulator::evaluationCount_` | A lock-free counter for a value many threads increment concurrently with no other coordination needed — the lightweight alternative to a mutex for exactly this shape of problem. |
| Level-based (not just topological) ordering | `ParallelSimulator::computeLevels()` | Groups mutually-independent work explicitly, instead of just picking *some* valid serial order — the actual prerequisite for correct parallelism. |
| `find_package(Threads REQUIRED)` | `backend/CMakeLists.txt` | The portable, standard CMake way to link a platform's thread library, instead of hardcoding `-pthread` or similar. |
| Artificial workload for demonstrating an optimization | `frontend/parallel_demo` | Honestly distinguishing "this mechanism works and scales" from "this specific toy program got faster" — the latter claim would be false for this project's actual gates. |

## Next up

**Phase 9 — Frontend (Dear ImGui):** still console-only — no way to
*see* a circuit or its signals, only printed text. Phase 9 adds a real
schematic/waveform view, wired to the backend built across Phases 1-8.
See `docs/ROADMAP.md` → "Why this order" for the full chain.
