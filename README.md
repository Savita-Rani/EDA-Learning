# EDA Tool — C++ Learning Project

A digital logic simulator (Electronic Design Automation tool), built from
C++ basics up through advanced concepts, with a native C++ frontend added
once the simulation core is solid.

This repo doubles as a **course**: every phase introduces new C++ concepts
in service of a real feature. See `docs/` for the full trail of decisions,
roadmap, and what-we-learned notes per phase.

## Current status

**Phase 1 — Gate basics** (console only). See `docs/concepts/phase1-basics.md`.

## Build & run

Requires: g++ (MinGW/MSYS2 — confirmed g++ 13.2.0), CMake >= 3.15.

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/bin/eda_console.exe
```

Or open the folder in VS Code (CMake Tools + C/C++ extensions) and use the
provided `.vscode/tasks.json` (build) and `.vscode/launch.json` (debug).

## Project layout

```
backend/              eda_core — the simulation engine (static library)
  include/eda/          public API, namespace eda:: (#include <eda/Gate.h>)
  src/                   implementation
frontend/
  console/              current UI — links against eda_core only
  (imgui/, qt/ later — same pattern: new sibling dir, link eda_core)
docs/                 roadmap, decisions, per-phase concept notes
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
