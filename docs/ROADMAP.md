# Roadmap

The project: a digital logic simulator (EDA tool). Backend and frontend both
in C++, starting from fundamentals and ending at advanced/systems-level C++.

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
| 2 | OOP | Gate hierarchy, `Wire`/`Signal` | inheritance, virtual/override, abstract base classes, polymorphism | ⏳ next |
| 3 | STL | Netlist as a graph | containers, iterators, algorithms, references vs pointers, topological sort | planned |
| 4 | Simulation engine | Event-driven scheduler | lambdas, `std::function`, `priority_queue`, Observer pattern | planned |
| 5 | File I/O | Netlist file format parser | streams, string parsing, exceptions | planned |
| 6 | Templates | Generic signal values (0/1/X/Z) | templates, generic programming, (C++20 concepts) | planned |
| 7 | Memory | Smart pointers, RAII, move semantics | `unique_ptr`/`shared_ptr`, rule of 5, move semantics | planned |
| 8 | Concurrency | Multithreaded simulation | `std::thread`, mutex, atomics, thread pools | planned |
| 9 | Frontend | Dear ImGui schematic + waveform view | linking external libs, UI event loop, backend/frontend separation | planned |
| 10 | Design patterns | Factory, Visitor, refined Observer | advanced OOP patterns, exception safety, const-correctness | planned |
| 11 | Testing | Expanded coverage, sanitizers, CI | ASan/UBSan, CI script (GoogleTest itself arrived early — see below) | planned |
| 12 | Performance | Profiling & optimization | cache-friendly layout, benchmarking | planned |

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
