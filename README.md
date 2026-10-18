# EDA Tool — C++ Learning Project

A digital logic simulator (Electronic Design Automation tool), built from
C++ basics up through advanced concepts, with a native C++ frontend added
once the simulation core is solid.

This repo doubles as a **course**: every phase introduces new C++ concepts
in service of a real feature. See `docs/` for the full trail of decisions,
roadmap, and what-we-learned notes per phase.

## Current status

**Phase 1 — Gate basics** (console only): `docs/phases/phase1.md`.
**Phase 2 — OOP / `Component` hierarchy** (new files, Phase 1 untouched):
`docs/phases/phase2.md`. Both phases coexist in the same build.

## Build, run, test

Requires: g++ (MinGW/MSYS2 — confirmed g++ 13.2.0), CMake >= 3.15.

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/bin/eda_console.exe          # Phase 1 frontend
./build/bin/eda_console_phase2.exe   # Phase 2 frontend
cd build && ctest --output-on-failure   # all tests, both phases (13 total)
```

Or open the folder in VS Code (CMake Tools + C/C++ extensions): build with
`Ctrl+Shift+B`, debug with `F5` (`.vscode/tasks.json` / `launch.json`).

See `docs/phases/phase1.md` / `phase2.md` for the full walkthrough (build
details, VS Code steps, what each test checks and why, troubleshooting).

## Project layout

```
backend/
  include/eda/Gate.h       Phase 1 public API, namespace eda::
  include/eda/Component.h  Phase 2 public API (separate hierarchy)
  src/Gate.cpp              -> eda_core (static library)
  src/Component.cpp         -> eda_oop (static library)
frontend/
  console/              Phase 1 UI — links eda_core only
  console_phase2/       Phase 2 UI — links eda_oop only
  (imgui/, qt/ later — same pattern: new sibling dir, link the backend lib)
tests/
  test_gate.cpp          -> eda_tests (Phase 1, GoogleTest)
  test_component.cpp      -> eda_component_tests (Phase 2, GoogleTest)
docs/
  ROADMAP.md            all 12 phases, concepts + status
  DECISIONS.md           dated log of choices + rationale
  phases/phase1.md, phase2.md, ...  one doc per phase
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
