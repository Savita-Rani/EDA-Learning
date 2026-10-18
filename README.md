# EDA Tool — C++ Learning Project

A digital logic simulator (Electronic Design Automation tool), built from
C++ basics up through advanced concepts, with a native C++ frontend added
once the simulation core is solid.

This repo doubles as a **course**: every phase introduces new C++ concepts
in service of a real feature. See `docs/` for the full trail of decisions,
roadmap, and what-we-learned notes per phase.

## Current status

**Phase 1 — Gate basics** (console only). Full build/run/test/troubleshooting
guide: `docs/phases/phase1.md`.

## Build, run, test

Requires: g++ (MinGW/MSYS2 — confirmed g++ 13.2.0), CMake >= 3.15.

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/bin/eda_console.exe      # run the frontend
cd build && ctest --output-on-failure   # run the backend's test suite
```

Or open the folder in VS Code (CMake Tools + C/C++ extensions): build with
`Ctrl+Shift+B`, debug with `F5` (`.vscode/tasks.json` / `launch.json`).

See `docs/phases/phase1.md` for the full walkthrough (build details, VS
Code steps, what the test checks and why, troubleshooting).

## Project layout

```
backend/              eda_core — the simulation engine (static library)
  include/eda/          public API, namespace eda:: (#include <eda/Gate.h>)
  src/                   implementation
frontend/
  console/              current UI — links against eda_core only
  (imgui/, qt/ later — same pattern: new sibling dir, link eda_core)
tests/                eda_tests — backend test suite, run via ctest
docs/
  ROADMAP.md            all 12 phases, concepts + status
  DECISIONS.md           dated log of choices + rationale
  phases/phase1.md, ...  one doc per phase: build/run/test/troubleshoot/concepts
.vscode/              tasks.json (build), launch.json (gdb debug), settings.json
build/                (gitignored) CMake build output -> build/bin/*.exe
```

**Backend/frontend separation is deliberate and enforced by the build, not
just convention:** `backend/` never includes anything from `frontend/`.
Any frontend only ever depends on `eda_core`'s public headers under
`backend/include/eda/`. See `docs/DECISIONS.md` (2026-10-18, "Backend/
frontend separation") for why, and the ImGui→Qt porting note.

## Roadmap

See `docs/ROADMAP.md` for all 12 phases, from `enum class` basics to
multithreaded simulation and an ImGui frontend.
