# Phase 1 — Gate basics

**Goal:** represent a logic gate as a C++ class and print its truth table
from the console. No inheritance yet, no file I/O, no UI — just enough to
anchor the first batch of fundamentals in real, runnable code.

**Status:** done. Builds clean, console output verified, GoogleTest suite
green (6/6).

## Files

- `backend/include/eda/Gate.h` — declares `GateType` (`enum class`) and
  `Gate` (class), inside `namespace eda`. This is the backend's public API.
- `backend/src/Gate.cpp` — implements `Gate`'s constructor and
  `evaluate()`/`name()`.
- `frontend/console/main.cpp` — builds one `Gate` per type, prints each
  truth table. Links against `eda_core`; includes only `<eda/Gate.h>`.
- `tests/test_gate.cpp` — GoogleTest `TEST()` cases for every gate's truth
  table, discovered individually by CTest. Links against `eda_core` and
  `GTest::gtest_main`, nothing else.

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

Phase 1 uses **GoogleTest** (`mingw-w64-ucrt-x86_64-gtest`, installed via
MSYS2's `pacman`, matching our ucrt64 g++ toolchain). Originally planned
for Phase 11, pulled forward since "how do I test this" deserves a real
framework answer from the start.

```powershell
cmake --build build --target eda_tests
cd build
ctest --output-on-failure
```

`gtest_discover_tests` (in `tests/CMakeLists.txt`) registers each `TEST()`
as its own CTest entry, so you'll see:

```
Start 1: GateTest.AND
1/6 Test #1: GateTest.AND .....................   Passed
Start 2: GateTest.OR
2/6 Test #2: GateTest.OR ......................   Passed
...
100% tests passed, 0 tests failed out of 6
```

Run the test binary directly for GoogleTest's own (more detailed) output:

```powershell
./build/bin/eda_tests.exe
```

```
[==========] Running 6 tests from 1 test suite.
[ RUN      ] GateTest.AND
[       OK ] GateTest.AND (0 ms)
...
[  PASSED  ] 6 tests.
```

**What each test actually checks:** for every `GateType`, `expected()` in
`test_gate.cpp` independently recomputes the truth table (written
differently from `Gate.cpp` — e.g. XOR is `(a||b) && !(a&&b)` in the test
vs. `a != b` in the implementation — so a copy-paste bug can't hide from
it), compares against `Gate::evaluate()`'s real output via `EXPECT_EQ`,
and checks `Gate::name()` via `EXPECT_STREQ` (not `EXPECT_EQ` — that would
compare `const char*` pointers, not string contents).

**Confirmed the suite actually catches bugs, not just passes trivially:**
temporarily broke `NAND` (`!(a && b)` → `(a && b)`), ran the tests, got a
precise failure —

```
Expected equality of these values:
  gate.evaluate(a, b)
    Which is: true
  expected(type, a, b)
    Which is: false
NAND(true, true)
[  FAILED  ] GateTest.NAND (7 ms)
[  PASSED  ] 5 tests.
```

— reverted, confirmed 6/6 green again.

## Troubleshooting

- **`find_package(GTest CONFIG REQUIRED)` fails ("Could not find a package
  configuration file")** — GoogleTest isn't installed for this toolchain.
  Install the ucrt64 package (matches our `C:/msys64/ucrt64/bin/c++.exe`):
  ```powershell
  C:\msys64\usr\bin\pacman.exe -S mingw-w64-ucrt-x86_64-gtest
  ```
  If `pacman` itself reports mirror errors (stale mirror DB is common on
  Windows MSYS2 installs) but still finishes with "installing
  mingw-w64-ucrt-x86_64-gtest..." at the end, it succeeded — one mirror
  came through despite the noise. Confirm with:
  `C:\msys64\usr\bin\pacman.exe -Qs gtest`.
- **`ctest` runs 0 tests after adding a new `TEST()`** — `gtest_discover_tests`
  runs `eda_tests.exe --gtest_list_tests` as part of the build, so you need
  `cmake --build build` to finish successfully first; a stale discovery
  cache from before the new TEST() existed can also linger — reconfigure
  (`cmake -S . -B build ...`) if a rebuild alone doesn't pick it up.
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
| Anonymous `namespace {}` | `test_gate.cpp` | Gives `expected()` and `expectTruthTable()` internal linkage — private to this translation unit, won't collide with same-named symbols elsewhere if the test file grows. |
| `CTest` (`enable_testing()`) | top-level `CMakeLists.txt` | Turns on CMake's test runner; `tests/CMakeLists.txt` then registers tests into it. |
| GoogleTest `TEST(Suite, Name)` | `test_gate.cpp` | Macro that defines and auto-registers a test case — no manual "list of tests to run" needed, unlike the Phase-1-original hand-rolled harness. |
| `EXPECT_EQ` vs `ASSERT_EQ` | `test_gate.cpp` | `EXPECT_*` records a failure and keeps running the rest of the test (so all 4 input combinations get checked even if one fails); `ASSERT_*` would bail out of the test immediately on the first failure. |
| `EXPECT_STREQ` | `test_gate.cpp` | For `const char*`, `EXPECT_EQ` would compare pointer addresses, not contents — `EXPECT_STREQ` compares the actual characters. A classic C-string gotcha. |
| `gtest_discover_tests` | `tests/CMakeLists.txt` | CMake module (`include(GoogleTest)`) that runs the test binary with `--gtest_list_tests` at build time and registers each `TEST()` as its own CTest entry, instead of one lump `add_test()`. |

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

See `docs/ROADMAP.md` → "Why this order" for the fuller causal chain: this
phase's actual limitation (new gate type means editing the `switch` in two
places) is specifically what Phase 2 fixes, and what Phase 2 leaves open
is specifically what Phase 3 fixes after it — the pattern continues
through all 12 phases.
