# Roadmap

The project: a digital logic simulator (EDA tool). Backend and frontend both
in C++, starting from fundamentals and ending at advanced/systems-level C++.
For the *why* behind this shape (vision, domain vocabulary, the phase-
isolation architecture rule), see `docs/PROJECT_IDEA.md` — this file is
the phase-by-phase detail that backs it up.

Each phase = one working increment + new C++ concepts introduced in service
of that increment. Status is kept in sync with the task list; this file is
the durable record of *what each phase covers and why*.

Each phase also gets its own doc under `docs/phases/phaseN.md` — build/run/
test/troubleshooting instructions plus the concepts table, kept with the
phase rather than buried in one growing file. Start there for "how do I run
this" questions; this file is for the cross-phase overview.

Cross-cutting topics that outgrow a single phase (a library, a tool, a
technique used across several phases) get their own doc under
`docs/notes/<topic>.md` instead — written for revision/interview prep, not
just "how this repo uses it." First one: `docs/notes/gtest.md`.

| # | Phase | Feature | New C++ concepts | Status |
|---|-------|---------|-------------------|--------|
| 0 | Scaffold | Project skeleton, CMake, git, docs | build systems, header/source split | ✅ done |
| 1 | Basics | `Gate` class, truth tables (console) | classes, `enum class`, `switch`, `const` methods, `std::vector` | ✅ done |
| 2 | OOP | `Component` hierarchy (new, separate from `Gate`) | inheritance, virtual/override, abstract base classes, polymorphism, `unique_ptr` preview | ✅ done |
| 3 | STL | Netlist as a graph (depends on Phase 2's `Component`) | `unordered_map`/`vector`/`queue`, topological sort (Kahn's algorithm), exceptions | ✅ done |
| 4 | Simulation engine | `EventSimulator` (parallel technique to `Netlist`, both depend on `Component`) | lambdas, `std::function`, `priority_queue`, Observer pattern, `unordered_set` dedup, delta-cycle batching | ✅ done |
| 5 | File I/O | `NetlistLoader` (depends on Phase 3's `Netlist`) | `std::istream`/`ifstream`, `getline`/`istringstream` parsing, exceptions, string→factory `unordered_map` | ✅ done |
| 6 | Templates | `LogicGate<T>` + `LogicValue` 0/1/X/Z (independent, like Phase 4) | class templates, C++20 concepts, requires-expressions, header-only-by-necessity | ✅ done |
| 7 | Memory | Smart pointers, RAII, move semantics | `unique_ptr`/`shared_ptr`, rule of 5, move semantics | ⏳ next |
| 8 | Concurrency | Multithreaded simulation | `std::thread`, mutex, atomics, thread pools | planned |
| 9 | Frontend | Dear ImGui schematic + waveform view | linking external libs, UI event loop, backend/frontend separation | planned |
| 10 | Design patterns | Factory, Visitor, refined Observer | advanced OOP patterns, exception safety, const-correctness | planned |
| 11 | Testing | Expanded coverage, sanitizers, CI | ASan/UBSan, CI script (GoogleTest itself arrived early — see below) | planned |
| 12 | Performance | Profiling & optimization | cache-friendly layout, benchmarking | planned |

## Why this order — the causal chain between phases

Not an arbitrary syllabus. Every phase exists because the *previous*
phase's design has exactly one limitation, and that phase's new C++
feature is the standard tool for closing it. Read top to bottom: each
row's "leaves open" is the next row's "fixes."

| # | Phase | Fixes (limitation left by the previous phase) | Leaves open (motivates the next phase) |
|---|---|---|---|
| 1 | Basics | *(starting point — nothing to fix yet)* | A `switch` over gate types means every new gate requires editing existing functions (`evaluate()` *and* `name()`). Doesn't scale. |
| 2 | OOP | New gate type = new class, zero edits elsewhere (polymorphism replaces the `switch`). | Gates are still isolated objects — nothing connects them into a circuit; no way to hold/iterate a *mix* of component types as a graph. |
| 3 | STL ✅ | A netlist (graph of `Component`s + wires, via `std::unordered_map`/`vector`) now evaluates itself automatically in correct order — `Netlist::simulate()` computes a topological sort (Kahn's algorithm) and runs every component once, in dependency order. Fixed Phase 2's "gates are isolated objects" gap. | `simulate()` does a **full recompute of every component, every call** — no notion of "only re-evaluate what a changed input actually affects," and no concept of discrete time, so sequential elements (flip-flops, clocked delay) have no way to exist. Fine for a handful of gates; doesn't scale to a large circuit re-simulated after one input flips. |
| 4 | Simulation engine ✅ | An event-driven scheduler (`EventSimulator`): only re-evaluates components downstream of an actual signal change, via a time-ordered event queue with same-instant events batched together — fixes Phase 3's full-recompute limitation directly. (Required a real fix mid-phase: naive one-event-at-a-time processing had a genuine race when two inputs changed simultaneously — see `docs/phases/phase4.md`.) | Circuits are still hand-built in C++ code (`addComponent` calls) — no way to describe a circuit as data/load one from a file. |
| 5 | File I/O ✅ | Netlists load from a text file (`NetlistLoader`) — describe a circuit as data, not code. Found a second real, toolchain-specific bug along the way (`ifstream::is_open()` vs `operator!`/`fail()` — see `docs/phases/phase5.md`). | Signals are hardcoded `bool` — no unknown/floating (`X`) or high-impedance (`Z`) states, no multi-bit buses. Already bit us in Phase 4 (a wire's "unset" state was indistinguishable from a genuinely-computed `false`). |
| 6 | Templates ✅ | Signal value type becomes generic (`LogicGate<T>`) — richer states (0/1/X/Z via `LogicValue`) possible without rewriting the engine per type; only 3 operations (AND/OR/NOT) needed per new type, NAND/NOR/XOR derived generically. No bug found this time — first phase in a row where testing confirmed correctness without catching a defect. | Netlists/components are heap-allocated through the hierarchy with no rigorous ownership story — who deletes what, and when, isn't yet governed by explicit rules. |
| 7 | Memory management | Smart pointers + rule-of-5 + move semantics make ownership explicit and leak-proof, formalizing what Phase 2's `unique_ptr` preview started informally. | Simulating a large netlist is single-threaded — independent parts of the circuit evaluate serially even though nothing requires that. |
| 8 | Concurrency | Independent subgraphs evaluate in parallel (thread pool, mutex/atomics for shared state) — real speedup on large circuits. | Still console-only — no way to *see* the circuit or its signals, only printed truth tables/logs. |
| 9 | Frontend (ImGui) | A real UI: schematic view + waveform viewer, wired to the backend built in phases 1-8. | The architecture (how gates/wires/the engine relate) has accreted informally across 8 phases — never named or cleaned up as patterns. |
| 10 | Design patterns | Factory (component creation), Visitor (netlist traversal), refined Observer (signal propagation) — the informal structures from earlier phases get named, cleaned up, made reusable. | By now the codebase is large enough that manual spot-checking isn't sufficient assurance of correctness. |
| 11 | Testing & tooling | Expanded test coverage across everything built so far, sanitizers (ASan/UBSan) for memory/UB bugs, CI scripting. (GoogleTest itself arrived early, in Phase 1 — this phase is about *scale* of coverage, not introducing the framework.) | Correctness is now trustworthy, but nothing has been measured for speed — premature optimization before this point would have been wasted effort on code that was still changing shape. |
| 12 | Performance | Profile real bottlenecks, optimize data layout for cache-friendliness, benchmark before/after. Last, deliberately — only after correctness is established. | *(end of planned roadmap)* |

**The thesis:** difficulty tracks necessity, not a fixed syllabus order —
each phase is "advanced" relative to the last specifically because the
simulator's own growing requirements demand it, not because harder topics
were inserted for their own sake. When a phase completes, its row above
gets updated with what was actually learned/fixed (see
`docs/phases/phaseN.md` for the detailed account); the next row's "why" is
rechecked against what actually got built, in case reality diverged from
the plan.

## Key decisions (see `docs/DECISIONS.md` for full rationale)

- **EDA tool type:** digital logic simulator (not analog/SPICE, not a
  schematic *editor* first) — best ratio of "teaches core C++ progressively"
  to "visibly working demo early."
- **Frontend:** console first, then **Dear ImGui** (not Qt, not web+C++) —
  pure C++, light setup, good fit for schematic/waveform rendering later.
  Kept portable to Qt later via the backend/frontend split below.
- **Learning style:** guided, step-by-step — concepts explained as
  introduced, small reviewable increments.
- **Toolchain:** g++ 13.2.0 (MSYS2/MinGW) + CMake 4.0 + git 2.43, confirmed
  present on this machine 2026-10-18. No MSVC (`cl`) installed.
- **IDE:** VS Code (C/C++ + CMake Tools extensions), not Qt Creator or
  Visual Studio — matches the existing MinGW/CMake toolchain.
- **Architecture:** `backend/` (static lib `eda_core`, namespace `eda::`)
  is fully independent of `frontend/`. Every frontend (console today,
  ImGui in Phase 9, Qt if ported later) is a sibling directory under
  `frontend/` that only includes `backend/include/eda/*.h` and links
  `eda_core` — enforced by the CMake dependency graph, not convention.
- **Testing:** GoogleTest was brought forward from Phase 11 into Phase 1
  (`mingw-w64-ucrt-x86_64-gtest` via pacman, `gtest_discover_tests` in
  `tests/CMakeLists.txt`) — real framework from day one instead of a
  throwaway hand-rolled harness. Phase 11 is now about expanding coverage,
  sanitizers, and CI, not introducing the framework.
- **Phase isolation:** starting with Phase 2, each phase's code lives in
  new files — never editing a previous phase's implementation. Phase 1's
  `Gate` and Phase 2's `Component` are separate classes, separate
  libraries (`eda_core` / `eda_oop`), separate frontends/tests, coexisting
  in the same build. Build-file changes are additive-only (verified via
  `git diff --stat`: insertions only, zero deletions). Lets every phase
  stay a standalone, runnable reference point for later comparison.
  Refined after Phase 3/4: "never edit" is not "never depend on" — reuse
  an earlier phase's class when building genuinely new capability on it
  (Phase 3 → Phase 2's `Component`), but keep a new phase independent
  when it's solving the *same* problem a previous phase solved, just
  differently (Phase 4's `EventSimulator` vs. Phase 3's `Netlist` — both
  depend on `Component`, neither depends on the other). Phase 5's
  `NetlistLoader` depends directly on Phase 3's `Netlist` (new capability
  — parsing — built on an existing piece, not a comparison). Phase 6's
  `LogicGate<T>`/`LogicValue` are independent too, same reasoning as
  Phase 4 (solves the same AND/OR/NOT/... problem Phase 1/2 solved, just
  generalized via templates — nothing to "build on").
