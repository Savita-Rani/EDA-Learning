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
./build/eda_sim.exe
```

## Project layout

```
include/   public headers (the "what")
src/       implementation + main() (the "how")
docs/      roadmap, decisions, per-phase concept notes
build/     (gitignored) CMake build output
```

## Roadmap

See `docs/ROADMAP.md` for all 12 phases, from `enum class` basics to
multithreaded simulation and an ImGui frontend.
