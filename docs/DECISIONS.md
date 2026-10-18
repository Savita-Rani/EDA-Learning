# Decision Log

Durable record of choices made and why, so we don't re-litigate them later
or lose the reasoning once the chat context rolls over.

---

### 2026-10-18 — EDA tool type: digital logic simulator

**Options considered:** digital logic simulator, schematic/circuit editor,
SPICE-like analog simulator.

**Decision:** digital logic simulator — gates, wires, netlists, event-driven
simulation, waveform output.

**Why:** Best balance of teaching core C++ concepts progressively (data
structures → graphs → event-driven engine → templates → concurrency) while
producing a visible, interactive demo almost immediately (toggle an input,
watch an output change). Analog/SPICE front-loads numerical methods before
any C++ fundamentals land; a schematic editor front-loads UI/graphics work.

---

### 2026-10-18 — Frontend: console → Dear ImGui (not Qt, not web)

**Options considered:** Qt, Dear ImGui, web frontend + C++ backend (via
WASM or local server/IPC).

**Decision:** Build the simulation core as a console app first. Once it's
solid, add **Dear ImGui** as the native frontend.

**Why:** User wants *both* backend and frontend in C++. Qt is the
industry-standard choice (KiCad uses it) but has a heavier setup/learning
curve that would compete with early C++ fundamentals. ImGui is lightweight,
pure C++, and quick to get a schematic/waveform view running. Web+C++
backend adds IPC/WASM complexity not needed for the learning goal. Starting
console-only avoids GUI concerns entirely until the simulation engine
(Phases 1-8) is solid.

---

### 2026-10-18 — Learning style: guided, step-by-step

**Decision:** Each new C++ concept is explained inline (via comments in the
code, plus chat) at the point it's introduced, in small increments the user
reviews before moving on — not a finished project handed over at once.

**Why:** Explicit user choice over "build fast" or "curriculum + project"
alternatives.

---

### 2026-10-18 — Toolchain

**Found on machine:** g++ 13.2.0 (MSYS2/MinGW), CMake 4.0.0-rc4, git 2.43.1.
No MSVC (`cl.exe`)/Visual Studio detected.

**Decision:** Build via CMake with the MinGW Makefiles generator, targeting
`g++`. C++ standard: C++20 (set in `CMakeLists.txt`) to have modern features
(concepts, ranges) available for later phases without needing to upgrade
mid-project.

---

### 2026-10-18 — IDE: VS Code (not Qt Creator, not Visual Studio)

**Options considered:** Qt Creator, VS Code, Visual Studio.

**Decision:** VS Code, with C/C++ and CMake Tools extensions.

**Why:** Qt Creator only pays off if the UI framework is Qt — we chose
ImGui instead (see frontend decision above), so Qt Creator buys nothing
over VS Code for a plain CMake + g++ project. Visual Studio steers toward
the MSVC toolchain, which isn't installed (only MinGW g++ is) — using it
would mean installing a large additional toolchain or fighting VS into
using MinGW. VS Code uses the existing g++/CMake/git setup with no changes.

---

### 2026-10-18 — Backend/frontend separation: `eda_core` static library

**Decision:** Split the project into `backend/` (built as a static library,
`eda_core`, namespace `eda::`) and `frontend/<name>/` (one directory per UI,
each linking against `eda_core` and including only its public headers under
`backend/include/eda/`). Today there's one frontend, `frontend/console`.

**Why:** User wants backend and frontend kept separate, with the explicit
intent to **port the frontend to Qt later** if needed, without reworking
the simulation engine. Making this a build-level boundary (not just a
folder naming convention) means:
- `backend/` can never accidentally `#include` something from `frontend/` —
  CMake's `add_subdirectory`/`target_link_libraries` graph only allows the
  dependency to run one way.
- Adding a Qt (or ImGui, Phase 9) frontend later is purely additive: a new
  `frontend/qt/` directory + a `CMakeLists.txt` that links `eda_core`. No
  changes to `backend/` are needed, by construction.
- The public API is explicit: anything frontends are allowed to touch lives
  under `backend/include/eda/` and is wrapped in `namespace eda`; anything
  under `backend/src/` is private implementation.

**How to apply:** every future phase's backend work (Phases 2-8: OOP
hierarchy, netlist, simulation engine, parsing, templates, memory,
concurrency) goes under `backend/`. UI-only work (Phase 9 ImGui, a possible
future Qt port) goes under a new `frontend/<name>/` sibling — never inside
`backend/`, and never with `backend/` including frontend headers.
