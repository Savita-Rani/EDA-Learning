# Phase 5 — File I/O (netlist parser)

**Goal:** fix Phase 4's remaining limitation — circuits were still only
ever hand-built via `addComponent()` calls in C++ source, with no way to
describe one as data. `loadNetlistFile()`/`loadNetlist()` parse a circuit
from a text file (or any `std::istream`) into a ready-to-use `Netlist`.

**Status:** done, after fixing one real, environment-specific bug found
by testing (not a contrived one — see below, the second phase in a row
to find one). Builds clean, demo produces correct output and demonstrates
error handling, GoogleTest suite green (7/7 new, 28/28 total across all
five phases).

## Relationship to earlier phases

`NetlistLoader` depends on Phase 3's `Netlist` (reused, not duplicated) —
new capability (parsing) built on an existing piece, the same
relationship Phase 3 had to Phase 2's `Component`. This is *not* a
parallel technique like Phase 4 was to Phase 3: there's nothing to
compare against here, parsing a file into a `Netlist` is genuinely new
functionality. See `docs/DECISIONS.md`, "Phase 5 depends on Phase 3."

Nothing in `Netlist.h`/`.cpp`, `EventSimulator.h`/`.cpp`, `Component.h`/
`.cpp`, `Gate.h`/`.cpp`, or any earlier frontend/test file was touched.
The three edited `CMakeLists.txt` got 39 insertions, 0 deletions.

## Files

- `backend/include/eda/NetlistLoader.h` / `.cpp` — `loadNetlist(istream&)`
  (the core parser) and `loadNetlistFile(path)` (opens a real file, calls
  the same parser). New `eda_fileio` static library, linking `eda_netlist`
  `PUBLIC`.
- `frontend/fileio_demo/main.cpp` + `CMakeLists.txt` + `circuits/*.nl` —
  loads the same 4-NAND XOR circuit as Phases 3/4's demos, now from a
  file (`circuits/xor4nand.nl`) instead of code, and deliberately also
  loads a broken file (`circuits/broken_unknown_gate.nl`) to show the
  error path firing for real.
- `tests/test_netlist_loader.cpp` — 7 GoogleTest cases, new
  `eda_fileio_tests` target.

## The netlist file format

One component per line, whitespace-separated:

```
GATE_TYPE OUTPUT_WIRE INPUT_WIRE1 [INPUT_WIRE2]
```

`GATE_TYPE` is one of `AND`/`OR`/`NOT`/`NAND`/`NOR`/`XOR`. `NOT` takes
exactly one input wire; everything else takes exactly two. Blank lines
and lines starting with `#` are ignored. Example
(`frontend/fileio_demo/circuits/xor4nand.nl`):

```
# 4-NAND decomposition of XOR
NAND n1 A B
NAND n2 A n1
NAND n3 B n1
NAND OUT n2 n3
```

**Deliberately excluded:** external input *values*. A netlist file
describes circuit *structure* only (same as real EDA netlist formats) —
supplying input values is simulation stimulus, still done in code via
`Netlist::setInput()` after loading. Keeping these separate avoids
conflating "what the circuit is" with "what we're testing it with."

## A second real bug, found by testing

Following Phase 4's precedent — this is an actual bug the test suite
caught, not a planted one.

**The bug:** `loadNetlistFile()` originally checked `if (!file)` after
constructing a `std::ifstream` from a nonexistent path, expecting the
standard-mandated behavior that a failed-to-open stream sets `failbit`
(making `operator!`/`fail()` report `true`). `NetlistLoaderTest.
ThrowsOnMissingFile` failed: no exception was thrown.

**Diagnosed with an isolated repro** (outside the test suite, outside
this project) on this exact toolchain — g++ 13.2.0, MSYS2 ucrt64:

```cpp
std::ifstream file("this_file_does_not_exist_12345.nl");
// is_open() == false  (correct — the file genuinely isn't open)
// fail()    == false  (wrong — should be true)
// bad()     == false
// eof()     == false
// !file     == false  (wrong — should be true)
```

`is_open()` correctly reports the failure; the stream-state operators
(`operator!`, `fail()`, `bad()`, `eof()`) do not, on this specific
toolchain. This is surprising (the standard says constructing from a
path that fails to open should set `failbit`), but reproducible and
consistent.

**The fix:** check `file.is_open()` instead of `!file`/`fail()`. This is
also simply the more direct, intention-revealing check regardless of the
toolchain quirk — "did opening this file succeed" is literally what
`is_open()` asks.

**Lesson, recorded for revision:** don't assume standard-mandated stream
behavior is portable in practice — prefer `is_open()` for this specific
check on any toolchain, not just as a workaround here.

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_fileio` (library), `eda_fileio_demo` (frontend),
`eda_fileio_tests` (tests). All prior targets (Phases 1-4) build
unchanged alongside these.

## How to run

```powershell
./build/bin/eda_fileio_demo.exe
```

```
=== EDA Logic Simulator — Phase 5: File I/O ===

Loading: .../frontend/fileio_demo/circuits/xor4nand.nl
  A  B  | OUT
  -------------
  0  0  |  0
  0  1  |  1
  1  0  |  1
  1  1  |  0

-- Now loading a deliberately broken netlist file --
Loading: .../frontend/fileio_demo/circuits/broken_unknown_gate.nl
Caught expected error: Unknown gate type: FOO
```

The circuit path is baked in at compile time via a `CIRCUIT_DIR`
preprocessor define set in `frontend/fileio_demo/CMakeLists.txt`
(`target_compile_definitions`), pointing at
`${CMAKE_CURRENT_SOURCE_DIR}/circuits` — works regardless of the
executable's current working directory, no file-copying into
`build/bin/` needed.

## How to test

```powershell
cd build
ctest --output-on-failure
```

7 new tests (`NetlistLoaderTest.*`), 28 total across all phases:

| Test | What it checks |
|---|---|
| `ParsesSimpleAndGate` | one-line circuit parses and simulates correctly |
| `ParsesFourNandXorAndSimulatesCorrectly` | the full 4-line XOR circuit, all 4 input combinations |
| `SkipsBlankLinesAndComments` | blank lines and `#` comments interspersed don't break parsing |
| `ThrowsOnUnknownGateType` | an unrecognized gate type (`FOO`) throws `std::runtime_error` |
| `ThrowsOnWrongInputCount` | too few inputs for a 2-input gate, and too many for `NOT`, both throw |
| `ThrowsOnMissingFile` | a nonexistent path throws — **the one that caught the `is_open()` bug** |
| `LoadsRealFileFromDisk` | writes a real temp file, loads it via `loadNetlistFile()`, cleans up — exercises the actual filesystem path, not just the in-memory stream parser |

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues. Specific to this phase:

- **Checking whether a file opened successfully** — use `.is_open()`,
  not `!file`/`.fail()`. See "A second real bug" above; this toolchain
  doesn't set `failbit` reliably on a failed-to-open `ifstream`.
- **Parser throws "Unknown gate type" unexpectedly** — gate type names
  are case-sensitive (`AND`, not `and`); check the netlist file for typos.

## What to notice when you run it

- `main.cpp` never calls `addComponent()` — the entire circuit comes from
  `circuits/xor4nand.nl`. Compare against Phase 3/4's demos, which build
  the identical circuit via 4 `addComponent()` calls in C++.
- The broken-file demonstration isn't a test-only concern — it's in the
  actual frontend, showing a real caller handling a real parse failure
  with `try`/`catch`, not just a unit test asserting on an exception type.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `std::istream` as a parameter type (not `std::ifstream`) | `NetlistLoader.h`/`.cpp` | Lets `loadNetlist()` parse from a real file OR an in-memory `std::istringstream` — same code, same tests work without touching the filesystem for most cases. |
| `std::getline` / `std::istringstream` tokenizing | `loadNetlist()` | Line-by-line, then whitespace-split each line via `>>` into an `istringstream` — standard text-parsing idiom. |
| Exceptions for malformed input | `NetlistLoader.cpp` | Unknown gate type, wrong input count, unreadable file — all throw `std::runtime_error` with a specific message (and line number, for in-file problems) instead of silently building a wrong circuit. |
| `std::ifstream::is_open()` vs `operator!`/`fail()` | `loadNetlistFile()` | Found to behave differently on this toolchain — `is_open()` is the correct, direct check; don't rely on stream-state operators for "did opening succeed." |
| `std::unordered_map<std::string, std::function<...>>` as a lookup table | `makeComponent()` | String → factory-function dispatch, replacing an if/else chain — an informal preview of the Factory pattern Phase 10 formalizes. |

## Next up

**Phase 6 — Templates:** signals are hardcoded `bool` throughout — no
unknown/floating (`X`) or high-impedance (`Z`) states, no multi-bit
buses. (This project already ran into the practical consequence of that
limitation in Phase 4 — a wire's "unset" state is indistinguishable from
a genuinely-computed `false`.) Phase 6 makes the signal value type
generic so richer states become possible without rewriting the engine
per type. See `docs/ROADMAP.md` → "Why this order" for the full chain.
