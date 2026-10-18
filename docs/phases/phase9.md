# Phase 9 — Frontend (Dear ImGui)

**Goal:** fix the limitation every phase since Phase 1 shared — console
output only, no way to actually *see* a circuit or its signals. Build a
real window: a live "schematic" panel (checkboxes for inputs, colored
live values for every wire) and a "waveform" panel (`OUT`'s value
plotted over time), wired to Phase 3's `Netlist`.

**Status:** done, with one honest caveat up front: this is the first
phase whose main deliverable (the rendered UI) **cannot be verified by
me** the way every previous phase's console output could — there's no
practical way to assert against rendered pixels, and I can't see a
window. What *was* verified: the app builds clean, launches a real
window, creates a real OpenGL context, runs its full render loop for a
fixed number of frames via an automated smoke-test mode, and shuts down
cleanly (exit code 0, no crash). **You should run
`./build/bin/eda_imgui_demo.exe` yourself and look at it** — that's the
only way to confirm the UI actually looks right. GoogleTest suite green
(4/4 new — the one piece of this phase that's actual logic — 70/70
total across all nine phases).

## Relationship to earlier phases

`main.cpp` depends on Phase 3's `Netlist` (reused, same relationship
Phase 5's `NetlistLoader` had) — new capability (an interactive GUI)
built on an existing piece, not a reimplementation of anything.
`WaveformBuffer` is new, independent, generic logic (same category as
Phase 7/8's infrastructure classes) — the one piece of this phase
that's practical to unit test.

**Dear ImGui, GLFW, and OpenGL are the first genuinely external
dependencies** this project has pulled in beyond GoogleTest. Nothing in
any earlier phase's files was touched. The three edited `CMakeLists.txt`
got 31 insertions, 0 deletions — the rest of the Phase 9 setup lives
entirely in the new `frontend/imgui_demo/CMakeLists.txt`.

## Files

- `backend/include/eda/WaveformBuffer.h` — fixed-capacity sample
  history, **header-only, no `.cpp` at all** (everything's inline) —
  an `INTERFACE` library in CMake (`eda_waveform`), not `STATIC`: there
  is nothing to compile, only an include path to propagate.
- `frontend/imgui_demo/main.cpp` + `CMakeLists.txt` — the actual GUI:
  fetches Dear ImGui via CMake's `FetchContent`, links GLFW (via
  pacman) and OpenGL (system `opengl32`), builds the window/render loop.
- `tests/test_waveform_buffer.cpp` — 4 GoogleTest cases, new
  `eda_waveform_tests` target.

## Why Dear ImGui is fetched as source, not linked as a library

Unlike GoogleTest (installed as a prebuilt package via `pacman`), Dear
ImGui has **no C++ package** on this toolchain (checked: only Python
bindings exist for `ucrt64`). This isn't a gap to work around — it's
intentional on ImGui's part: the library is explicitly designed to be
compiled directly into your own project (there's no "ImGui.dll" users
link against). `frontend/imgui_demo/CMakeLists.txt` uses CMake's
`FetchContent` to `git clone` a pinned release tag (`v1.93.0` — checked
against GitHub's actual latest release at the time, not guessed from
memory) at configure time, then compiles ImGui's core + the
GLFW/OpenGL3 backend files into a local `imgui` static library target.
Deliberately **not** given this project's `-Wall -Wextra -Wpedantic`
flags — those are for code we own; a vendored third-party library isn't
ours to "fix" warnings in.

**Dependencies needed, and how each was obtained:**

| Dependency | Source | Why |
|---|---|---|
| Dear ImGui | `FetchContent`, pinned to `v1.93.0` | No package exists; this is the library's own intended integration method |
| GLFW | `pacman -S mingw-w64-ucrt-x86_64-glfw` | Windowing + input; has a proper CMake package (`find_package(glfw3 CONFIG)`) |
| OpenGL | System (`opengl32` — confirmed `libopengl32.a` present in the MinGW toolchain's `lib/`) | Always available on Windows; no extra install needed |

A fresh clone of this repo needs internet access on its **first**
`cmake` configure, to fetch ImGui — same category of requirement
GoogleTest already had (needing `pacman` access), just via git instead
of a package mirror.

## What this phase does and doesn't unit-test

**Does:** `WaveformBuffer` — pure logic (capacity, drop-oldest
behavior, ordering), fully covered by GoogleTest, zero dependency on
ImGui/GLFW.

**Doesn't:** the actual rendering — `ImGui::Checkbox`, `PlotLines`,
`TextColored`, the GLFW window/event loop. There's no meaningful
assertion to write against "does this frame look right" without a
human looking at it (or a much heavier investment in pixel-diffing
infrastructure this project doesn't need). This isn't a gap papered
over — it's a deliberate, stated scope boundary, the same honesty this
project applied to e.g. Phase 8's artificial speedup numbers.

**What stands in for testing here:** an automated smoke-test mode.

## The `--max-frames=N` smoke-test mode

```powershell
./build/bin/eda_imgui_demo.exe --max-frames=60
```

Runs the full window/GL-context/ImGui lifecycle for exactly 60 frames,
then closes itself and exits — specifically so this could be verified
without a human watching the window or needing to manually close it.
Confirmed:

```
Exited cleanly after 60 frames.
exit code: 0
```

This proves: window creation succeeded, the OpenGL context was created,
ImGui/GLFW/OpenGL3 backends initialized without error, the render loop
ran 60 full iterations calling into `Netlist::setInput`/`simulate`/
`wireValue` and `WaveformBuffer::push` without crashing, and shutdown
(`ImGui_ImplOpenGL3_Shutdown`, etc.) completed cleanly. It does **not**
prove the UI is laid out sensibly, the colors are legible, or the plot
looks like a sensible waveform — **run it without `--max-frames` and
look** to confirm that.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

First configure will be noticeably slower than previous phases (~1
minute observed) — that's `FetchContent` cloning Dear ImGui's repo, a
one-time cost per fresh `build/` directory (it's cached inside
`build/_deps/`, itself gitignored along with the rest of `build/`).

## How to run

```powershell
./build/bin/eda_imgui_demo.exe
```

Opens a window titled "EDA Logic Simulator -- Phase 9: ImGui Frontend"
with two panels: **Schematic** (checkboxes for `A`/`B`, colored live
values for every wire in the 4-NAND XOR circuit) and **Waveform**
(`OUT`'s value plotted over the last 200 frames). Toggle the checkboxes
and watch both panels update live. Close the window normally to exit.

## How to test

```powershell
cd build
ctest --output-on-failure
```

4 new tests (`WaveformBufferTest.*`), 70 total across all phases — all
for `WaveformBuffer`; see "What this phase does and doesn't unit-test"
above for the rendering-code gap and why it's deliberate.

## Troubleshooting

- **`FetchContent` fails to clone ImGui (no internet / git error)** —
  this phase's one hard external requirement. Check `git ls-remote
  https://github.com/ocornut/imgui.git` works from this machine.
- **`find_package(glfw3 CONFIG REQUIRED)` fails** — GLFW isn't
  installed for this toolchain. Install with
  `pacman -S mingw-w64-ucrt-x86_64-glfw` (watch for the same
  mirror-noise-but-still-succeeds pattern documented for GoogleTest in
  `docs/phases/phase1.md` — confirm with `pacman -Qs glfw` rather than
  trusting the error-message noise alone).
- **Window opens but appears blank/black** — likely a graphics driver
  issue unrelated to this project's code; confirm `--max-frames=1`
  still exits cleanly (proves the app itself is fine) before debugging
  drivers.
- **Link errors mentioning `opengl32`** — confirm
  `C:\msys64\ucrt64\lib\libopengl32.a` exists; it ships with the base
  MinGW toolchain, not a separate package.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| Immediate-mode GUI loop | `main.cpp`'s `while` loop | Every frame rebuilds the *entire* UI from scratch via `ImGui::Checkbox`/`PlotLines` calls — nothing persists in ImGui itself between frames; only plain C++ variables we keep (`inputA`, `outHistory`) carry state. A fundamentally different shape than any prior phase's `main()`, which ran once. |
| Vendoring a third-party library as source | `frontend/imgui_demo/CMakeLists.txt` | `FetchContent` + compiling ImGui's own `.cpp` files ourselves — the library's own intended usage pattern, not a workaround. |
| `INTERFACE` library (CMake) | `backend/CMakeLists.txt`, `eda_waveform` | For a header-only target with literally nothing to compile — distinct from `STATIC` (compiles a `.cpp`) and from header-only templates (Phase 6/7, which still needed a *library* wrapping them for the include path, same shape here). |
| Separating what's testable from what isn't | `WaveformBuffer` vs. the render loop | Factoring the one piece of real logic out from the rendering specifically so it has genuine test coverage, rather than either skipping tests entirely or writing hollow ones against rendering calls. |
| Pinning a fetched dependency's version | `GIT_TAG v1.93.0` | Reproducibility — "whatever's newest on GitHub today" would make the build non-deterministic across time. |

## Next up

**Phase 10 — Design patterns:** the architecture (how gates/wires/the
engine relate) has accreted informally across 9 phases — Factory,
Visitor, and a refined Observer pattern get named and cleaned up
explicitly, rather than existing only as ad hoc code shapes. See
`docs/ROADMAP.md` → "Why this order" for the full chain.
