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
