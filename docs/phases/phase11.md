# Phase 11 — Testing & tooling

**Goal:** fix Phase 10's leftover limitation — the codebase (10 phases,
86 tests at the start of this phase) is large enough that manual
spot-checking isn't sufficient assurance of correctness. GoogleTest
itself arrived early (Phase 1), so this phase is specifically about
*expanding* coverage with real measurement, sanitizers, and CI — not
introducing a framework.

**Status:** done. Original finding was that AddressSanitizer/
UndefinedBehaviorSanitizer don't link on this project's MinGW/ucrt64
toolchain — true, and still the case for that specific toolchain. But
rather than stopping there, the user asked to shift to Visual Studio
specifically to get working sanitizer support, and that path was
pursued and **verified end-to-end**: MSVC (already installed on this
machine — VS2022 Community, C++ workload) ships a working
AddressSanitizer, confirmed by running this project's entire test suite
clean under it (90/90) and by proving ASan actually catches real bugs
with a deliberate standalone repro. Code coverage measurement, 4 new
tests closing real measured gaps, a verified local CI script, and a
GitHub Actions workflow with all three paths — are also done and
verified where verification was possible.

## Sanitizers: unavailable on the primary toolchain, verified working via MSVC

**First, the primary toolchain, checked directly (not assumed):**

```
$ g++ -fsanitize=address -g test.cpp -o test.exe
...cannot find -lasan: No such file or directory

$ g++ -fsanitize=undefined -g test.cpp -o test.exe
...cannot find -lubsan: No such file or directory
```

No `libasan`/`libubsan` anywhere in the MSYS2 installation, no
`clang64` environment installed. This part of the original finding
stands — this project's MinGW/ucrt64 toolchain genuinely cannot run
sanitizers, confirmed against both a minimal standalone repro and this
project's actual `eda_tests` target.

**Then: the user asked to shift to Visual Studio to get sanitizers
working, instead of accepting the limitation.** VS2022 Community (with
the C++ workload, including MSVC's AddressSanitizer support since
VS2019 16.9+) turned out to already be installed on this machine — no
new install needed. Added as a **parallel opt-in path**, not a
toolchain replacement: the normal MinGW build is completely unaffected,
and the new path only exists when deliberately invoked.

**What had to change to make this work:**

- `CMakeLists.txt`'s `EDA_ENABLE_SANITIZERS` option branches on
  `if(MSVC)`: `/fsanitize=address /Zi` instead of
  `-fsanitize=address,undefined ...` — MSVC's flag is ASan-only, no
  UBSan equivalent exists for MSVC.
- `frontend/*` subdirectories are wrapped in `if(NOT MSVC) ... endif()`
  — the pacman-installed GLFW and GoogleTest are built for the MinGW
  ABI and aren't link-compatible with MSVC's different ABI/CRT. None of
  this project's actual tests need any frontend demo, so they're simply
  skipped under MSVC rather than porting GLFW/ImGui to it, which
  nothing here needs.
- `tests/CMakeLists.txt`'s GoogleTest acquisition branches on
  `if(MSVC)` too: fetches GoogleTest v1.15.2 from source via
  `FetchContent` (same technique already used for Dear ImGui in
  Phase 9) instead of the pacman package, for the same ABI reason.
- `scripts/sanitize-msvc.ps1` (new): locates the VS2022 install via
  `vswhere.exe` (rather than hardcoding a version-specific path),
  configures a separate `build-msvc-sanitize/` directory with the
  Visual Studio generator and `-DEDA_ENABLE_SANITIZERS=ON`, builds, and
  runs `ctest`.

**Two real environment-specific quirks hit and resolved while actually
running this — not anticipated in advance:**

1. **`STATUS_DLL_NOT_FOUND`** (exit code `-1073741515`) — the ASan
   runtime (`clang_rt.asan_dynamic-x86_64.dll`) ships next to `cl.exe`,
   under `VC\Tools\MSVC\<version>\bin\Hostx64\x64`, but isn't on `PATH`
   by default, so instrumented binaries failed to even start. Fixed by
   prepending that directory to `$env:PATH` in the script.
2. **"Interception failure" abort** — `AddressSanitizer: CHECK failed:
   interception_win.cpp:171 ... Interception failure` — ASan's own
   Windows API hooking (how it intercepts `malloc`/etc. to track
   allocations) conflicted with something already present in this
   sandboxed environment. Fixed by setting
   `ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE=1`, exactly as suggested
   by ASan's own error message.

With both fixed, a full clean run from scratch:

```
=== Configure (Visual Studio 17 2022, x64, AddressSanitizer) ===
...
=== Build ===
...
=== Test (under AddressSanitizer) ===
...
100% tests passed, 0 tests failed out of 90
=== ASan run PASSED (90/90 as of Phase 11) ===
```

**Proof ASan is actually catching things, not just passing silently** —
a standalone throwaway repro (not committed; `scratch_asan_check.cpp`),
a deliberate heap-buffer-overflow:

```cpp
int* arr = new int[5];
arr[5] = 42;   // one past the end — deliberate heap overflow
```

compiled and run under the exact same MSVC `/fsanitize=address` path,
produced a full ASan diagnostic report (heap-buffer-overflow, with the
allocation site and the exact out-of-bounds write caught), then was
deleted once it had served its purpose of proving the instrumentation
is real and working, not a no-op.

**A real PowerShell bug found and fixed along the way, unrelated to
ASan itself:** `scripts/sanitize-msvc.ps1` failed to parse, with
cascading "Unexpected token" / "string is missing the terminator"
errors reported at unrelated line numbers. Root cause, found by
systematic bisection (isolating snippets, narrowing by truncation,
then a byte-level scan comparing against this project's other, working
scripts): an em-dash (`—`, U+2014) **inside an active string literal**
(not inside a `#` comment, which is unaffected either way) on line 36,
in a file with no UTF-8 BOM, broke PowerShell 5.1's parser on this
machine. Fixed by using a plain ASCII hyphen instead. A real encoding
gotcha worth remembering: non-ASCII characters are safe in PowerShell
comments but not reliably safe inside live strings without a BOM.

**Bonus finding, not pursued further:** building this project's code
directly under MSVC surfaced a `C4456` warning (local variable hides a
previous local variable of the same name) that GCC doesn't warn about
at its current flags — a real example of different compilers catching
different things, noted but not acted on since it's not a bug, just a
style warning this project doesn't currently enforce under MSVC.

**Decision:** keep both the honest "doesn't work on the primary
toolchain" finding *and* the "works via MSVC" finding — they're not in
conflict, they're sequential. `EDA_ENABLE_SANITIZERS` stays a single
CMake option that branches by compiler; `scripts/sanitize-msvc.ps1` is
the verified way to actually exercise it on this machine;
`.github/workflows/ci.yml`'s `sanitize` (ubuntu-latest) job still
exists separately because it adds UBSan coverage that MSVC's ASan-only
support doesn't provide.

## Code coverage: works, and produced real, measured numbers

`--coverage` (GCC's built-in `gcov` instrumentation, not a separate
runtime) works cleanly on this toolchain. `EDA_ENABLE_COVERAGE=ON`
turns it on; `scripts/coverage-report.ps1` runs `gcov` against every
`backend/src/*.cpp` object, parses the output, and reports per-file and
aggregate line coverage — filtering out system headers, vendored
ImGui, and GoogleTest's own code, which aren't this project's to
measure or fix.

**First real run, before any new tests:**

| File | Coverage |
|---|---|
| `FileHandle.cpp` | 68.18% |
| `ComponentVisitor.cpp` | 82.35% |
| `NetlistLoader.cpp` | 89.58% |
| `Gate.cpp` | 90.00% |
| `ComponentFactory.cpp` | 93.10% |
| `LogicValue.cpp` | 93.55% |
| everything else | 98.61%-100% |

**Aggregate: 371/393 lines (94.40%).**

Read the actual uncovered lines (not just the percentages) for the
three lowest files:

- **`FileHandle.cpp`**: `operator=(FileHandle&&)` — move **assignment**
  — was never exercised by any test. Phase 7's own `test_file_handle.cpp`
  only covered move **construction**. A real, genuine gap.
- **`ComponentVisitor.cpp`**: `visitOr`/`visitNot`/`visitNor` dispatch
  branches never exercised. Phase 10's own `test_component_visitor.cpp`
  only covered AND/NAND/XOR/BUFFER/unknown. A real, genuine gap.
- **`NetlistLoader.cpp`**: the `OR`/`NOT`/`NOR`/`XOR` factory branches
  in `makeComponent()`, and the "missing output wire" parse-error path,
  never exercised. Phase 5's own `test_netlist_loader.cpp` only parsed
  AND/NAND gates and a couple of unrelated error cases. A real, genuine
  gap.

## Closing the gaps — without editing earlier phases' test files

All three gaps are in **earlier phases' code**, discovered via **this
phase's** tooling. This project's isolation rule (never edit a previous
phase's files) still applies — so the fix isn't "add these cases to
`test_file_handle.cpp`" etc. Instead: `tests/test_coverage_expansion.cpp`
is a brand-new file, added by this phase, containing exactly the
missing cases, linking the three existing libraries it needs
(`eda_memory`, `eda_patterns`, `eda_fileio`) the same way any frontend
demo links multiple backend libraries. See `docs/DECISIONS.md`, "Phase
11 closes coverage gaps in new files, not by editing earlier phases'
tests."

**Re-measured after adding the 4 new tests:**

| File | Before | After |
|---|---|---|
| `FileHandle.cpp` | 68.18% | **100%** |
| `ComponentVisitor.cpp` | 82.35% | **100%** |
| `NetlistLoader.cpp` | 89.58% | **100%** |

**New aggregate: 386/393 lines (98.22%).**

## What's left uncovered, and why — checked, not left unexplained

Seven lines remain uncovered. Checked each one directly rather than
stopping at the percentage:

- **`Gate.cpp`** (2 lines) and **`LogicValue.cpp`** (2 lines): defensive
  `return` statements after an exhaustive `switch` over every enum
  value — unreachable by construction, not a real gap. (Explicitly
  commented `// unreachable` in the source already, from Phases 1 and
  6 respectively.)
- **`ComponentFactory.cpp`** (2 lines): `gcov` marks these `=====`, not
  `#####` — its notation for compiler-generated exception-unwinding
  scaffolding (the implicit cleanup path if something threw while
  destroying a local object during a `return`), not reachable from
  any normal test.
- **`ParallelSimulator.cpp`** (1 line): `std::this_thread::sleep_for(simulatedGateCost_)`
  — only runs when `simulatedGateCost_ > 0`. Every test uses the
  default zero-cost constructor deliberately, to keep the suite fast;
  only Phase 8's demo uses a nonzero cost (and demos don't run under
  `ctest`/`gcov`). This one actually *could* be covered trivially, and
  isn't — a deliberate choice (a real wall-clock delay in the test
  suite, for a near-zero-value assertion that `sleep_for` was called at
  all) rather than an oversight.

**Final, fully-explained aggregate: 386/393 (98.22%)**, with every one
of the 7 remaining uncovered lines individually accounted for. Not "we
hit 100%" (impossible given the unreachable/compiler-generated lines),
and not "coverage is good" as an unexamined number — the actual reason
for every gap is known.

## Local CI script — verified both outcomes, not just the happy path

`scripts/ci.ps1`: configure → build → `ctest`, exit code propagating
properly. Confirmed both ways it needs to work:

- **Pass path**: ran it clean, all 90 tests passed, printed
  `=== CI PASSED ===`, exit code `0`.
- **Fail path**: temporarily broke `Gate::evaluate`'s `AND` case
  (`a && b` → `a || b`), reran — printed `=== CI FAILED at: Test ===`
  with the specific failing test (`GateTest.AND`) and a nonzero exit
  code. Reverted, reran, confirmed `=== CI PASSED ===` again.

`-Coverage` flag also runs the coverage configuration and calls
`scripts/coverage-report.ps1` automatically — run standalone and
confirmed it reproduces the exact numbers above.

## GitHub Actions workflow — written, NOT verified (no remote)

`.github/workflows/ci.yml` — three jobs:

- `windows` (MSYS2 ucrt64, mirrors the local toolchain exactly).
- `windows-asan` — runs `scripts/sanitize-msvc.ps1` directly on
  `windows-latest`'s pre-installed MSVC, reusing the exact script
  verified locally rather than duplicating its logic in YAML.
- `sanitize` (`ubuntu-latest`) — kept even though MSVC's ASan now
  works, because Linux GCC's sanitizer runtimes also give UBSan, which
  MSVC's sanitizer support doesn't provide at all.

**This repo has no remote configured** — the workflow has never been
triggered, because there's no GitHub repository for GitHub Actions to
run against. Written to be correct based on everything verified
locally this phase; "written correctly" and "confirmed working in CI"
are different claims, and only the first one is true right now. Said
plainly in the file's own header comment too, not just here.

## Relationship to earlier phases

`tests/test_coverage_expansion.cpp` depends on `eda_memory`,
`eda_patterns`, `eda_fileio` (all reused, not duplicated) — closing
coverage gaps in existing code, not new capability requiring its own
library. `scripts/`, `.github/`, and the top-level `CMakeLists.txt`
options are pure tooling, with no code-separation concern at all (not
backend/frontend/tests code attributable to a specific earlier phase).

Nothing in any earlier phase's files was touched. `backend/CMakeLists.txt`
wasn't touched at all this phase (no new backend library). The two
build files edited for coverage/sanitizer options (`CMakeLists.txt`,
`tests/CMakeLists.txt`) only gained MSVC-specific branches and the
frontend `if(NOT MSVC)` guard — additive, nothing removed from the
MinGW path.

## Files

- `CMakeLists.txt` — `EDA_ENABLE_COVERAGE`/`EDA_ENABLE_SANITIZERS`
  options (appended), the latter branching on `if(MSVC)`; `frontend/*`
  wrapped in `if(NOT MSVC)`.
- `tests/CMakeLists.txt` — `eda_coverage_tests` target (4 tests) +
  `if(MSVC)` branch to fetch GoogleTest from source instead of the
  MinGW-ABI pacman package.
- `tests/test_coverage_expansion.cpp` — the 4 new tests.
- `scripts/ci.ps1` — local CI pipeline, verified pass and fail paths.
- `scripts/coverage-report.ps1` — automates the per-file/aggregate gcov
  analysis above.
- `scripts/sanitize-msvc.ps1` — locates VS2022 via `vswhere.exe`,
  configures/builds/tests under MSVC AddressSanitizer in a separate
  `build-msvc-sanitize/` directory. Verified: 90/90 tests pass.
- `.github/workflows/ci.yml` — three jobs (`windows`, `windows-asan`,
  `sanitize`); unverified (no remote), see above.
- `.gitignore` — added `build-coverage/`, `build-sanitize/`,
  `build-msvc-sanitize/`, `*.gcov`/`*.gcda`/`*.gcno`, `*.pdb`.

## How to build and run

```powershell
# Normal build/test (unaffected by this phase's options — both default OFF):
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
cd build; ctest --output-on-failure

# One-command version of the above, with proper exit code:
powershell -File scripts/ci.ps1

# Coverage build + report:
powershell -File scripts/ci.ps1 -Coverage
# ...or manually:
cmake -S . -B build-coverage -G "MinGW Makefiles" -DEDA_ENABLE_COVERAGE=ON
cmake --build build-coverage
cd build-coverage; ctest; cd ..
powershell -File scripts/coverage-report.ps1

# Sanitizer build on the PRIMARY toolchain (confirmed to fail — see above):
cmake -S . -B build-sanitize -DEDA_ENABLE_SANITIZERS=ON
cmake --build build-sanitize   # fails at link: cannot find -lasan / -lubsan

# Sanitizer build via MSVC (verified working — see above):
powershell -File scripts/sanitize-msvc.ps1
```

## How to test

```powershell
cd build
ctest --output-on-failure
```

4 new tests, 90 total across all phases.

| Test | What it checks |
|---|---|
| `CoverageExpansionTest.FileHandleMoveAssignmentClosesPreviousFileFirst` | the gap that mattered most — move-assigning into a `FileHandle` that already owns an open file closes the old one first, doesn't leak it |
| `CoverageExpansionTest.VisitorDispatchesOrNotNorCorrectly` | the three missing `ComponentVisitor` dispatch branches |
| `CoverageExpansionTest.NetlistLoaderParsesEveryGateTypeNotJustAndNand` | `NetlistLoader` actually handles all 6 gate types, not just the 2 previously exercised |
| `CoverageExpansionTest.NetlistLoaderThrowsOnLineMissingOutputWire` | the previously-untested malformed-line error path |

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues.

- **`-DEDA_ENABLE_SANITIZERS=ON` fails to link on the MinGW build** —
  expected on this toolchain (see above); not a setup mistake. Use
  `scripts/sanitize-msvc.ps1` for a verified local ASan run, or the
  `ubuntu-latest` CI job (once/if this repo gets a remote) for ASan+UBSan.
- **`scripts/sanitize-msvc.ps1` exits with `-1073741515`
  (`STATUS_DLL_NOT_FOUND`)** — the ASan runtime DLL isn't on `PATH`.
  The script handles this itself by locating and prepending the MSVC
  toolset's `bin\Hostx64\x64` directory; if this still happens, confirm
  the VS2022 C++ workload (with the AddressSanitizer component) is
  actually installed.
- **`scripts/sanitize-msvc.ps1` aborts with an "Interception failure"**
  — ASan's Windows API hooking conflicting with something else in the
  environment; the script already sets
  `ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE=1` to work around this —
  if it still happens, something about the hooking conflict has
  changed and needs fresh investigation.
- **A `.ps1` script fails to parse with "Unexpected token" / "string is
  missing the terminator" at seemingly unrelated line numbers** — check
  for non-ASCII characters (em-dashes, etc.) inside active string
  literals, not just comments, especially in a file with no UTF-8 BOM.
  Exactly this broke `scripts/sanitize-msvc.ps1` once (see above);
  replacing the character with a plain ASCII equivalent fixed it.
- **`scripts/coverage-report.ps1` finds no data** — run the coverage
  build and `ctest` first (see "How to build and run" above); the
  script only reads already-generated `.gcda` files, it doesn't build
  anything itself.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| `gcov`/`--coverage` | `CMakeLists.txt`, `scripts/coverage-report.ps1` | GCC's built-in coverage instrumentation — no separate runtime library needed, unlike sanitizers, which is part of why it worked on the primary toolchain immediately and sanitizers didn't. |
| Sanitizers (ASan/UBSan) | `CMakeLists.txt`'s `EDA_ENABLE_SANITIZERS` option, `scripts/sanitize-msvc.ps1` | What they catch (memory errors, undefined behavior) that review and coverage can't. Unavailable on this project's primary MinGW toolchain (no `libasan`/`libubsan`) but **verified working via MSVC's AddressSanitizer** — same CMake option, different compiler branch. A real lesson: "unavailable" is a property of a *toolchain*, not of the *tool* — worth checking an alternate toolchain before writing something off as a hard limitation. |
| Closing a measured gap vs. assuming coverage is fine | `test_coverage_expansion.cpp` | The entire phase's thesis: *measure*, don't guess, then fix what's real and explain what isn't. |
| Distinguishing real gaps from unreachable/compiler-generated code | the "what's left uncovered" accounting above | 100% coverage is not always achievable *or desirable* — knowing precisely why a line isn't covered matters more than the raw percentage. |
| CI as a script first, workflow YAML second | `scripts/ci.ps1` vs. `.github/workflows/ci.yml` | The local script is independently useful and was fully verified; the YAML just orchestrates the same idea on a hosted runner, and inherits the local script's correctness rather than duplicating its logic. |
| Encoding gotchas in plain-text scripts | the em-dash/BOM bug in `scripts/sanitize-msvc.ps1`, found and fixed this phase | A non-ASCII character is safe in a comment but not reliably safe inside a live string literal without a UTF-8 BOM — a real bug, not a hypothetical one, caught by systematic bisection rather than guessing. |

## Next up

**Phase 12 — Performance:** correctness is now measured and
trustworthy; nothing has been measured for *speed* yet. Premature
optimization before this point would have been wasted effort on code
that was still changing shape every phase. Phase 12 profiles real
bottlenecks and optimizes with evidence, not guesses — deliberately
last. See `docs/ROADMAP.md` → "Why this order" for the full chain.
