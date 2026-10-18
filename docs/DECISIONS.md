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

---

### 2026-10-18 — Lightweight CTest smoke test now, not deferred entirely to Phase 11

**Decision (superseded same day — see next entry):** Add
`tests/test_gate.cpp` + `tests/CMakeLists.txt` now — plain asserts wired
into CMake's `enable_testing()`/`add_test()`, runnable via `ctest`. No
framework (Catch2/GoogleTest) yet; that's still Phase 11.

**Why:** User asked for "how to run, test, and other aspects" to be
documented for Phase 1 — a test section needs something real to describe,
not just "look at the console output." A zero-dependency CTest-based smoke
test is cheap to add, gives every future phase a regression check as
`Gate`/the netlist/the simulation engine evolve, and the same `ctest` entry
point survives unchanged when Phase 11 adds a real framework alongside it.
Verified it actually catches regressions (temporarily broke `AND`'s
implementation, confirmed the test failed with the right diagnostic,
reverted, confirmed green again) rather than trusting it blindly.

**How to apply:** superseded — see "GoogleTest brought forward to Phase 1"
below. The pattern (link `eda_core` only, every backend-touching phase
adds/extends tests under `tests/`) still holds.

---

### 2026-10-18 — GoogleTest brought forward to Phase 1 (supersedes plain-assert harness)

**Decision:** Replace the plain-assert `tests/test_gate.cpp` with real
GoogleTest `TEST()` cases, same day it was written. Installed
`mingw-w64-ucrt-x86_64-gtest` 1.14.0 via MSYS2's `pacman` — matches our
existing ucrt64 g++ toolchain exactly, so `find_package(GTest CONFIG
REQUIRED)` resolves with no extra `CMAKE_PREFIX_PATH` hints. Wired
`gtest_discover_tests` (not a single `add_test()`) so each `TEST()` shows
up as its own CTest entry (`GateTest.AND`, `GateTest.OR`, ...).

**Why:** User explicitly asked to bring GoogleTest into Phase 1 rather
than waiting for Phase 11. A real framework from the start beats a
hand-rolled harness that Phase 11 would've thrown away anyway — no reason
to write two test harnesses for the same code. `pacman`'s mirror list was
stale (many 404s against outdated mirrors) but one mirror still succeeded;
worth knowing that pacman's "too many errors, skipping" noise on a
multi-mirror failure isn't necessarily a hard failure — check `pacman -Qs`
to confirm before assuming it didn't install.

**How to apply:** Phase 11 ("Testing & tooling") now means *expanding*
coverage, CI scripting, and sanitizers (ASan/UBSan) — not introducing a
framework, since GoogleTest is already in place. Every future
backend-touching phase adds `TEST()` cases to `tests/`, same pattern:
link `eda_core` + `GTest::gtest_main`, no new `find_package` needed once
it's found.

---

### 2026-10-18 — Phase isolation: each phase in new files, never edit a previous phase

**Decision:** Starting with Phase 2, every phase's code lives in new
files. Phase 1's `Gate` (`backend/include/eda/Gate.h`,
`backend/src/Gate.cpp`, `frontend/console/main.cpp`,
`tests/test_gate.cpp`) is never edited again — Phase 2 introduces a
completely separate `Component` hierarchy (new header, new source, new
static library `eda_oop`, new frontend `frontend/console_phase2/`, new
tests `tests/test_component.cpp`), covering the same six gates a
different way. Both phases' binaries and tests coexist in one build (13
tests total: 6 `GateTest.*` + 7 `ComponentTest.*`).

Build-file changes are kept additive-only wherever the new code needs
wiring in: new `add_library`/`add_executable`/`add_subdirectory` blocks
appended below existing ones, never a modified line in an existing block.
Verified via `git diff --stat` on all three touched `CMakeLists.txt`
files: 37 insertions, 0 deletions.

**Why:** User explicitly asked, starting Phase 2: "for phase 2 i will
need separate code do not edit phase 1." Keeps each phase a standalone,
runnable reference point — e.g. the switch-based `Gate` and the
polymorphic `Component` can be compared side by side, rather than the
earlier implementation being overwritten and lost to history (even though
git history would technically preserve it, having both runnable
*simultaneously* is more useful for this project's revision/comparison
purpose than archaeology through commits).

**How to apply:** every future phase (3-12) follows this pattern — new
class names/files for new techniques, new frontend directories for new
demos if needed, new test files, additive-only build file changes. If a
later phase's design seems to genuinely require changing an earlier
phase's file (e.g. a future netlist phase wanting `Gate` itself to
change), stop and ask the user explicitly first rather than assuming —
that would contradict this default.
