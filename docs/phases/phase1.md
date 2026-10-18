# Phase 1 — Gate basics

**Goal:** represent a logic gate as a C++ class and print its truth table
from the console. No inheritance yet, no file I/O, no UI — just enough to
anchor the first batch of fundamentals in real, runnable code.

**Status:** done. Builds clean, console output verified, smoke test green.

## Files

- `backend/include/eda/Gate.h` — declares `GateType` (`enum class`) and
  `Gate` (class), inside `namespace eda`. This is the backend's public API.
- `backend/src/Gate.cpp` — implements `Gate`'s constructor and
  `evaluate()`/`name()`.
- `frontend/console/main.cpp` — builds one `Gate` per type, prints each
  truth table. Links against `eda_core`; includes only `<eda/Gate.h>`.
- `tests/test_gate.cpp` — smoke test for every gate's truth table, wired
  into CTest. Links against `eda_core`, nothing else.

(Originally `Gate.h`/`Gate.cpp`/`main.cpp` lived under flat `include/`/
`src/`; restructured into `backend/`/`frontend/console/` immediately after
— see `docs/DECISIONS.md`, "Backend/frontend separation" — to make the
separation a build-level guarantee instead of a convention, in time for a
later Qt port if wanted.)

## How to build

One-time configure, then build whenever files change:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

This produces three targets:

| Target | What it is | Output |
|---|---|---|
| `eda_core` | the backend, static library | `build/backend/libeda_core.a` |
| `eda_console` | console frontend | `build/bin/eda_console.exe` |
| `eda_tests` | smoke test | `build/bin/eda_tests.exe` |

**In VS Code:** open the folder, let the CMake Tools extension configure
(reads `.vscode/settings.json`, which pins the MinGW Makefiles generator),
then `Ctrl+Shift+B` runs the `CMake: build` task.

Rebuilding is incremental — `cmake --build build` only recompiles files
that changed. You only need to rerun the `cmake -S . -B build ...`
configure step after adding/removing a `CMakeLists.txt` or a source file
listed in one (CMake needs to regenerate the Makefiles in that case).

## How to run

```powershell
./build/bin/eda_console.exe
```

Prints all six gates' truth tables (AND/OR/NOT/NAND/NOR/XOR) to the
console. No input/arguments needed yet — Phase 1 hardcodes one instance of
each gate type and exercises both inputs' 4 combinations.

**In VS Code:** press **F5** (uses `.vscode/launch.json`, config "Debug
eda_console (gdb)"). This builds first (`preLaunchTask`), then launches
under gdb — set a breakpoint in `Gate::evaluate` or `main()` beforehand to
step through it.

## How to test

```powershell
cmake --build build --target eda_tests
cd build
ctest --output-on-failure
```

or run the test binary directly for more detail:

```powershell
./build/bin/eda_tests.exe
```

Expect:

```
30/30 checks passed
```

(6 gates × (1 name check + 4 input combinations) = 30.) Exit code `0` on
success, `1` if any check fails, with `FAIL: <description>` lines printed
for each failing check on stderr.

**What this test actually checks:** for every `GateType`, it independently
recomputes the expected truth table (written differently from
`Gate.cpp` — e.g. XOR is computed as `(a||b) && !(a&&b)` in the test vs.
`a != b` in the implementation — so a copy-paste bug can't hide from it)
and compares against `Gate::evaluate()`'s real output, plus checks
`Gate::name()` returns the right string.

**Why this isn't Catch2/GoogleTest yet:** that's deliberately deferred to
Phase 11 ("Testing & tooling"), once there's enough backend surface area
to justify pulling in a framework. For now, plain asserts + `add_test()` in
`tests/CMakeLists.txt` get you a real, CI-friendly `ctest` pass/fail with
zero dependencies — and this harness carries forward unchanged when Phase
11 swaps in a framework (same CTest entry point, more tests added beside
it).

**Confirmed the harness actually catches bugs, not just passes
trivially:** during setup, `AND`'s implementation was temporarily broken
(`a && b` → `a || b`); the test caught it immediately (`28/30 checks
passed`, 2 explicit `FAIL:` lines), then passed again after reverting.

## Troubleshooting

- **"CXX compiler identification" fails / wrong compiler picked** — make
  sure `g++` from MSYS2/MinGW is on `PATH` (`g++ --version` should print
  `13.2.0` or similar). If CMake finds a different compiler than expected,
  delete `build/` and reconfigure — CMake caches the compiler choice in
  `build/CMakeCache.txt` and won't re-detect it otherwise.
- **Added a new `.cpp` file but the build doesn't pick it up** — you need
  to list it explicitly in the relevant `CMakeLists.txt` (`add_executable`/
  `add_library` call) and rerun the configure step (`cmake -S . -B build
  ...`), not just `cmake --build build`.
- **Stale/corrupted build after restructuring files** — safe to delete the
  whole `build/` directory and reconfigure from scratch; it's gitignored
  and 100% regenerable:
  ```powershell
  Remove-Item -Recurse -Force build
  cmake -S . -B build -G "MinGW Makefiles"
  cmake --build build
  ```
- **VS Code doesn't show IntelliSense for `<eda/Gate.h>`** — make sure the
  CMake Tools extension has configured at least once (so
  `compile_commands.json` exists in `build/`); `C_Cpp.default.
  configurationProvider` in `.vscode/settings.json` points the C/C++
  extension at it.
- **gdb not found when pressing F5** — confirm `gdb --version` works in a
  terminal; it ships with MSYS2/MinGW (`C:\msys64\ucrt64\bin\gdb.exe` on
  this machine).

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `#pragma once` | `Gate.h` | Header guard — prevents double-inclusion errors when multiple files `#include "Gate.h"`. |
| `enum class` | `Gate.h` | Strongly-typed enum: no silent int conversion, no name collisions across enums (vs plain C `enum`). |
| class vs struct | `Gate.h` | `class` defaults to private members — used here because `type_` is an implementation detail, not part of the public interface. |
| Header/source split | `Gate.h` + `Gate.cpp` | Declaration (*what* a Gate can do) is separated from definition (*how*). Callers only need to recompile if the header changes, not the implementation. |
| Member initializer list | `Gate.cpp` constructor | `Gate::Gate(GateType type) : type_(type) {}` initializes `type_` directly instead of default-constructing then assigning in the body. |
| `const` member functions | `Gate.h`/`Gate.cpp` | `evaluate()`/`name()` promise not to modify the object — lets you call them on `const Gate&` references. |
| Default arguments | `Gate.h` | `evaluate(bool a, bool b = false)` lets `NOT` gates be called with just one argument. |
| `switch` on `enum class` | `Gate.cpp` | Exhaustive dispatch over gate types; compilers can warn if a case is missing. |
| `std::vector<T>` | `main.cpp` | Dynamically-sized container holding one `Gate` per type. |
| Range-based `for` | `main.cpp` | `for (const auto& gate : gates)` — no manual indexing, `const auto&` avoids copying each `Gate`. |
| `std::cout` streams | `main.cpp` | `<<` insertion operator; note `bool` prints as `1`/`0` by default. |
| `namespace eda` | `Gate.h`, `Gate.cpp` | Everything the backend exposes lives in `eda::`. It's the literal boundary a frontend talks across — added when we split into `backend/`/`frontend/` so the public API is explicit, not just "whatever's in the header." |
| `using namespace eda;` | `main.cpp` | Pulls `eda::` names into scope for this one file. Fine in a small `.cpp`; never do this in a header — it would leak into every file that includes it. |
| Anonymous `namespace {}` | `test_gate.cpp` | Gives `expected()`, `check()`, `checkCount`/`failCount`, and `testGate()` internal linkage — each is private to this translation unit, won't collide with same-named symbols elsewhere if the test file grows. |
| `CTest` (`enable_testing()` / `add_test()`) | top-level `CMakeLists.txt`, `tests/CMakeLists.txt` | Registers `eda_tests` as a named test (`GateTruthTables`) runnable via plain `ctest` — the same mechanism Phase 11's real framework will plug into. |

## What to notice when you run it

- NOT gate's truth table repeats the same OUT for `b=false` and `b=true` —
  because `evaluate()` ignores `b` for NOT. That's the default-argument
  trick in action, not a bug.
- Every other gate's table should match the standard 2-input truth table
  you'd find in any digital logic textbook.

## Open questions / things we're deliberately deferring

- No inheritance yet — every gate type lives inside one `Gate` class via a
  `switch`. Phase 2 introduces a `Component` base class with virtual
  `evaluate()`, and this `switch`-based version becomes the "before" to
  compare against the polymorphic "after."
- No wires/connections yet — gates are evaluated standalone with manually
  supplied inputs. Phase 3 introduces `Wire`/`Signal` and a netlist graph.

## Next up

**Phase 2 — OOP:** replace the `switch`-based `Gate` with a `Component`
base class and virtual `evaluate()`, one derived class per gate type.
`tests/test_gate.cpp`'s independently-recomputed expected values carry
forward unchanged — they'll validate the polymorphic version against the
exact same truth tables, which is the point: behavior shouldn't change,
only how it's implemented.
