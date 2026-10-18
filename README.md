# EDA Tool — C++ Learning Project

A digital logic simulator (Electronic Design Automation tool), built from
C++ basics up through advanced concepts, with a native C++ frontend added
once the simulation core is solid.

This repo doubles as a **course**: every phase introduces new C++ concepts
in service of a real feature. See `docs/` for the full trail of decisions,
roadmap, and what-we-learned notes per phase.

**New here? Start with `docs/PROJECT_IDEA.md`** — the project's vision,
domain vocabulary (gate/wire/netlist/signal/wiring), scope (logic
simulation only, not physical layout or analog/SPICE), and the
architecture rule (phase isolation + depend-vs-independent) that shapes
every file in this repo. This file below is build/run/test instructions
and a status snapshot, not the "why."

## Current status

- **Phase 1 — Gate basics** (console only): `docs/phases/phase1.md`.
- **Phase 2 — OOP / `Component` hierarchy** (new files, Phase 1 untouched):
  `docs/phases/phase2.md`.
- **Phase 3 — Netlist (STL)** (new files; *depends on* Phase 2's
  `Component` — see `docs/phases/phase3.md` for why that's reuse, not an
  edit): `docs/phases/phase3.md`.
- **Phase 4 — Event-driven engine** (new files; depends on `Component`,
  deliberately independent of Phase 3's `Netlist` — a parallel technique
  for the same problem, found and fixed a real race-condition bug along
  the way): `docs/phases/phase4.md`.
- **Phase 5 — File I/O** (new files; depends on Phase 3's `Netlist` —
  new capability, not a parallel technique; found and fixed a second
  real, toolchain-specific bug): `docs/phases/phase5.md`.
- **Phase 6 — Templates** (new files; independent of `Component`/`Gate`,
  same reasoning as Phase 4 — generalizes the same gate behaviors via a
  C++20-concept-constrained template, `LogicGate<T>`): `docs/phases/phase6.md`.
- **Phase 7 — Memory management** (new files; independent, generic
  infrastructure — hand-written `OwningPtr<T>`/`SharedOwningPtr<T>`/
  `FileHandle`, demonstrating RAII surviving a real thrown exception):
  `docs/phases/phase7.md`.
- **Phase 8 — Concurrency** (new files; `ThreadPool` independent,
  `ParallelSimulator` depends on it + `Component` — level-based parallel
  evaluation, verified race-free across 20 repeated test runs, honest
  artificial-workload speedup demo): `docs/phases/phase8.md`.
- **Phase 9 — Frontend (Dear ImGui)** (new files; depends on Phase 3's
  `Netlist`; first genuinely external dependencies — ImGui fetched as
  source via `FetchContent`, GLFW via pacman, OpenGL from the system;
  first phase whose main deliverable needs a human to actually look at
  it — see its doc for what was and wasn't automatically verified):
  `docs/phases/phase9.md`.
- **Phase 10 — Design patterns** (new files; `ComponentFactory`/
  `ComponentVisitor` depend on `Component`, `Signal<Args...>`
  independent — Visitor via external `dynamic_cast` dispatch since
  `Component` can't be edited to add `accept()`): `docs/phases/phase10.md`.

All ten coexist in the same build.

## Build, run, test

Requires: g++ (MinGW/MSYS2 — confirmed g++ 13.2.0), CMake >= 3.15,
internet access on first configure (to fetch Dear ImGui).

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/bin/eda_console.exe          # Phase 1 frontend
./build/bin/eda_console_phase2.exe   # Phase 2 frontend
./build/bin/eda_netlist_demo.exe     # Phase 3 frontend
./build/bin/eda_event_demo.exe       # Phase 4 frontend
./build/bin/eda_fileio_demo.exe      # Phase 5 frontend
./build/bin/eda_generic_demo.exe     # Phase 6 frontend
./build/bin/eda_memory_demo.exe      # Phase 7 frontend
./build/bin/eda_parallel_demo.exe    # Phase 8 frontend
./build/bin/eda_imgui_demo.exe       # Phase 9 frontend — the first real GUI; run this one yourself and look
./build/bin/eda_patterns_demo.exe    # Phase 10 frontend
cd build && ctest --output-on-failure   # all tests, all phases (86 total)
```

Or open the folder in VS Code (CMake Tools + C/C++ extensions): build with
`Ctrl+Shift+B`, debug with `F5` (`.vscode/tasks.json` / `launch.json`).

See `docs/phases/phase1.md` through `phase10.md` for the full walkthrough
(build details, VS Code steps, what each test checks and why,
troubleshooting).

## Project layout

```
backend/
  include/eda/Gate.h            Phase 1 public API, namespace eda::
  include/eda/Component.h       Phase 2 public API (separate hierarchy)
  include/eda/Netlist.h         Phase 3 public API (depends on Component)
  include/eda/EventSimulator.h  Phase 4 public API (depends on Component,
                                  independent of Netlist — parallel technique)
  include/eda/NetlistLoader.h   Phase 5 public API (depends on Netlist)
  include/eda/LogicValue.h      Phase 6 public API part 1 (independent)
  include/eda/LogicGate.h       Phase 6 public API part 2 — header-only
                                  template, no corresponding .cpp (see
                                  docs/phases/phase6.md for why)
  include/eda/OwningPtr.h       Phase 7 — header-only template (independent)
  include/eda/SharedOwningPtr.h Phase 7 — header-only template (independent)
  include/eda/FileHandle.h      Phase 7 — ordinary class (independent)
  include/eda/ThreadPool.h      Phase 8 — independent, generic infrastructure
  include/eda/ParallelSimulator.h Phase 8 — depends on ThreadPool + Component
  include/eda/WaveformBuffer.h  Phase 9 — INTERFACE lib (no .cpp at all, independent)
  include/eda/BufferGate.h       Phase 10 — new Component subclass (depends on Component)
  include/eda/ComponentFactory.h Phase 10 — depends on Component
  include/eda/ComponentVisitor.h Phase 10 — depends on Component + BufferGate
  include/eda/Signal.h           Phase 10 — INTERFACE lib, independent (variadic template)
  src/Gate.cpp              -> eda_core (static library)
  src/Component.cpp         -> eda_oop (static library)
  src/Netlist.cpp           -> eda_netlist (static library, links eda_oop)
  src/EventSimulator.cpp    -> eda_event (static library, links eda_oop)
  src/NetlistLoader.cpp     -> eda_fileio (static library, links eda_netlist)
  src/LogicValue.cpp        -> eda_generic (static library)
  src/FileHandle.cpp        -> eda_memory (static library)
  src/ThreadPool.cpp        -> eda_threadpool (static library)
  src/ParallelSimulator.cpp -> eda_parallel (static library, links
                                eda_oop + eda_threadpool)
  src/BufferGate.cpp,
  src/ComponentFactory.cpp,
  src/ComponentVisitor.cpp   -> eda_patterns (static library, links eda_oop)
frontend/
  console/              Phase 1 UI — links eda_core only
  console_phase2/       Phase 2 UI — links eda_oop only
  netlist_demo/         Phase 3 UI — links eda_netlist (transitively eda_oop)
  event_demo/           Phase 4 UI — links eda_event (transitively eda_oop)
  fileio_demo/           Phase 5 UI — links eda_fileio; circuits/*.nl data files
  generic_demo/          Phase 6 UI — links eda_generic
  memory_demo/           Phase 7 UI — links eda_memory AND eda_oop (for Component
                           to own; doesn't make eda_memory itself depend on it)
  parallel_demo/         Phase 8 UI — links eda_parallel
  imgui_demo/            Phase 9 UI — links eda_netlist + eda_waveform + the
                           fetched imgui lib (ImGui/GLFW/OpenGL); see its
                           CMakeLists.txt for the FetchContent setup
  patterns_demo/         Phase 10 UI — links eda_patterns + eda_signal
  (qt/ later, if ever ported — same pattern: new sibling dir, link the backend lib)
tests/
  test_gate.cpp             -> eda_tests (Phase 1, GoogleTest)
  test_component.cpp         -> eda_component_tests (Phase 2, GoogleTest)
  test_netlist.cpp            -> eda_netlist_tests (Phase 3, GoogleTest)
  test_event_simulator.cpp     -> eda_event_tests (Phase 4, GoogleTest)
  test_netlist_loader.cpp       -> eda_fileio_tests (Phase 5, GoogleTest)
  test_logic_value.cpp,
  test_logic_gate.cpp            -> eda_generic_tests (Phase 6, GoogleTest)
  test_owning_ptr.cpp,
  test_shared_owning_ptr.cpp,
  test_file_handle.cpp            -> eda_memory_tests (Phase 7, GoogleTest)
  test_thread_pool.cpp              -> eda_threadpool_tests (Phase 8, GoogleTest)
  test_parallel_simulator.cpp        -> eda_parallel_tests (Phase 8, GoogleTest)
  test_waveform_buffer.cpp             -> eda_waveform_tests (Phase 9, GoogleTest —
                                           the only part of Phase 9 with test coverage)
  test_buffer_gate.cpp,
  test_component_factory.cpp,
  test_component_visitor.cpp           -> eda_patterns_tests (Phase 10, GoogleTest)
  test_signal.cpp                       -> eda_signal_tests (Phase 10, GoogleTest)
docs/
  PROJECT_IDEA.md       the vision/why, read this first
  ROADMAP.md            all 12 phases, concepts + status + why (causal chain)
  DECISIONS.md           dated log of choices + rationale
  phases/phase1.md ... phase10.md  one doc per phase
  notes/gtest.md, ...    cross-cutting reference notes (revision/interview prep)
.vscode/              tasks.json (build), launch.json (gdb debug), settings.json
build/                (gitignored) CMake build output -> build/bin/*.exe
```

**Each phase lives in its own files — a later phase never edits an
earlier one's.** Phase 1's `Gate`/`eda_core` and Phase 2's
`Component`/`eda_oop` are separate classes, separate libraries, separate
frontends and tests, all coexisting in one build. See `docs/DECISIONS.md`
(2026-10-18, "Phase isolation") for why.

**Backend/frontend separation is deliberate and enforced by the build, not
just convention:** `backend/` never includes anything from `frontend/`.
Any frontend only ever depends on `eda_core`'s public headers under
`backend/include/eda/`. See `docs/DECISIONS.md` (2026-10-18, "Backend/
frontend separation") for why, and the ImGui→Qt porting note.

## Roadmap

See `docs/ROADMAP.md` for all 12 phases, from `enum class` basics to
multithreaded simulation and an ImGui frontend.
