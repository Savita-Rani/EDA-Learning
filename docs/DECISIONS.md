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

---

### 2026-10-18 — Phase 3 depends on Phase 2 (not a violation of phase isolation)

**Decision:** `Netlist` (Phase 3) stores `std::unique_ptr<Component>` —
Phase 2's class, used as-is, not duplicated. New library `eda_netlist`
links `eda_oop` (`PUBLIC`). No file belonging to Phase 2 (or Phase 1) was
edited; `Netlist.h`/`Netlist.cpp` are entirely new files.

**Why this is different from Phase 2's relationship to Phase 1:** Phase 2
was a deliberate *parallel reimplementation* of Phase 1's `Gate` — same
behavior, built twice, specifically so the switch-based and polymorphic
versions could be compared side by side. Phase 3 is not a reimplementation
of anything; it's new functionality (a netlist/graph) that the roadmap's
own plan (and Phase 2's "Next up" note, written before Phase 3 started)
said should be built *using* `Component`, because that's the exact
interface a heterogeneous collection of gate types needs. The
phase-isolation rule (`docs/DECISIONS.md`, "Phase isolation" above) says
never *edit* a previous phase's files — it was never a rule against
*depending on* one when the plan calls for it. Reusing `Component` here
is the payoff Phase 2 was explicitly built to set up.

**How to apply:** when a later phase's design genuinely calls for reusing
an earlier phase's class/library (as opposed to reimplementing it for
comparison, like Phase 2 did), link against it — don't duplicate it just
to force file-level independence. The rule to keep protecting is "never
edit," not "never depend on." If it's ambiguous which situation a new
phase is in, check whether the *previous* phase's own "Next up" section
already called for this dependency (it did, here) — if not, ask before
assuming reuse is intended over reimplementation.

---

### 2026-10-18 — Phase 4 is a parallel technique, not a Netlist extension

**Decision:** `EventSimulator` (Phase 4) depends on Phase 2's
`Component` (reused, same pattern as Phase 3) but is deliberately
**independent of** Phase 3's `Netlist` — a new `eda_event` library, not
an extension of `eda_netlist`. Both solve the identical problem
(propagate values correctly through wired-up `Component`s); they do it
with different techniques (full topological recompute vs. event-driven
incremental re-evaluation), and both remain runnable side by side.

**Why this is the Phase-2-to-Phase-1 pattern, not the Phase-3-to-Phase-2
pattern:** Phase 3 built genuinely *new* capability (a graph/netlist)
*using* `Component` — reuse was correct because nothing comparable to
`Netlist` existed yet to compare against. Phase 4 is different: Phase 3's
`Netlist::simulate()` and Phase 4's `EventSimulator::run()` both answer
the same question ("what are all the wire values after this input
change") — exactly the kind of same-behavior-different-technique
situation Phase 2 vs. Phase 1 established the precedent for. Extending
`Netlist` in place to add event-driven behavior would have erased the
"before" to compare against, the same way editing `Gate` instead of
writing `Component` would have in Phase 2.

**How to apply:** when a new phase's job is "solve the same problem a
previous phase solved, differently," treat it like Phase 2 (new,
independent implementation) even if it reuses a *lower-level* building
block (here, `Component`) that both techniques legitimately share. When
a new phase's job is "build something that didn't exist before, using an
existing piece," treat it like Phase 3 (depend on it directly). The
building block being shared (`Component`) is not itself what's being
compared; the comparison is at the level of `Netlist` vs
`EventSimulator`.

**Also recorded here — the real bug this phase found:** the first
version of `EventSimulator::run()` processed one event at a time instead
of batching same-instant events, producing a genuine race condition (two
simultaneous input changes to the same gate could be applied out of
order, with the wrong one winning). Caught by `tests/test_event_simulator.cpp`
on first run — not a planted bug. Fixed with "delta cycle" batching (see
`docs/phases/phase4.md`, "A real bug, found by testing" for the full
account). Noted here as a reminder that this project's testing habit
(verify by demonstration) catches more than contrived regressions when
the code under test is genuinely new.

---

### 2026-10-18 — Phase 5 depends on Phase 3 (reuse, not a parallel technique)

**Decision:** `NetlistLoader` (Phase 5) depends on Phase 3's `Netlist` —
`loadNetlist()`/`loadNetlistFile()` parse a text file directly into a
`Netlist`. New `eda_fileio` library, linking `eda_netlist` `PUBLIC`.
Nothing in `Netlist.h`/`.cpp` was edited.

**Why this is the Phase-3-to-Phase-2 pattern, not the Phase-4-to-Phase-3
pattern:** parsing a circuit description into a `Netlist` is genuinely
*new* functionality — there's no prior "thing" to compare it against the
way `EventSimulator` was compared against `Netlist`. This is reuse of an
existing piece to build something that didn't exist before, exactly like
`Netlist` reusing `Component`.

**Note — could have targeted `EventSimulator` instead, or both:** the
parser's output is deliberately a generic `Netlist`, not tied to
`EventSimulator`'s wiring shape, even though both share the same
`addComponent(component, inputWires, outputWire)` signature. Chose
`Netlist` specifically because the roadmap phase description says
"netlist file format parser," and `Netlist`/`EventSimulator` are
interchangeable enough (same wiring shape) that adding a second loader
targeting `EventSimulator` later, if ever needed, would be a small,
independent addition — not a reason to complicate this phase now.

---

### 2026-10-18 — `ifstream::is_open()` vs `operator!`/`fail()`: a real, toolchain-specific bug

**Finding:** on this project's toolchain (g++ 13.2.0, MSYS2 ucrt64),
constructing a `std::ifstream` from a path that fails to open does
**not** reliably set `failbit`. `is_open()` correctly reports `false`,
but `fail()`, `bad()`, `eof()`, and `operator!` all report "no problem"
regardless. Confirmed via an isolated repro outside this project's code
(plain `g++ -std=c++20` on a nonexistent filename) — not specific to this
project's parser logic.

**Caught by:** `tests/test_netlist_loader.cpp`'s `ThrowsOnMissingFile`,
which failed on first run — `loadNetlistFile()` silently returned an
empty `Netlist` for a nonexistent path instead of throwing.

**Decision:** always check `.is_open()` after constructing an
`std::ifstream` from a filename in this project, never `!file`/`.fail()`
alone, for the specific "did opening succeed" question. `is_open()` is
also simply more direct regardless of the toolchain quirk.

**How to apply:** this isn't limited to `NetlistLoader` — any future
phase that opens a file (e.g., if a future save/export feature is added)
should use the same check. Worth remembering as a general lesson: don't
assume standard-mandated iostream behavior holds on every toolchain
without verifying.

---

### 2026-10-18 — Phase 6 is independent, like Phase 2 and 4

**Decision:** `LogicValue`/`LogicGate<T>` (Phase 6) are a new,
independent hierarchy — not built on `Component`/`Gate`, no dependency on
either. New library `eda_generic`.

**Why:** `LogicGate<bool>` solves the exact same problem Phase 1's `Gate`
and Phase 2's `Component` solve (AND/OR/NOT/NAND/NOR/XOR truth tables),
just generalized via templates to also support `LogicValue`. Same
reasoning as Phase 4 vs. Phase 3: same-problem-different-technique means
independent, not a dependency — there'd be nothing to meaningfully
"build on" here; `LogicGate<T>` isn't new capability layered on
`Component`, it's a more general replacement technique for the same
capability.

**How to apply:** continues the pattern established after Phase 4 — ask
"is this phase solving a NEW problem using an old building block (→
depend), or the SAME problem a previous phase solved, differently (→
independent)?" before wiring up a new phase's dependencies.

---

### 2026-10-18 — Templates require header-only definitions; concepts over SFINAE

**Finding:** `LogicGate<T>` (a class template) cannot be split into
`.h`/`.cpp` the way every previous phase's classes were — the compiler
needs the complete definition at every instantiation site
(`LogicGate<bool>`, `LogicGate<LogicValue>`, each in different
translation units), which a separately-compiled `.cpp` can't provide.
`LogicGate.h` is header-only; only `LogicValue.cpp` (ordinary, non-template
functions) gets compiled into `eda_generic`.

**Decision:** constrain `LogicGate<T>` with a C++20 concept
(`LogicType`) rather than leaving it an unconstrained template or using
pre-C++20 SFINAE/`enable_if`. Verified the payoff directly: compiled
`LogicGate<int>` standalone and confirmed the diagnostic names the exact
unsatisfied constraint (`LogicType`) at the exact instantiation site,
rather than a wall of substitution-failure noise from inside
`evaluate()`'s body. Also confirmed a subtler point while doing this:
the failure path was "found the `bool` overload via `int`→`bool`
implicit conversion, but it returns the wrong type," not "found no
overload at all" — demonstrating why the concept's `-> std::same_as<T>`
return-type check matters, not just "is this expression well-formed."

**How to apply:** when a future phase needs a generic/templated
component, constrain it with a concept expressing the actual required
operations, and verify the rejection path actually produces a useful
diagnostic (don't just assume it does).

---

### 2026-10-18 — Phase 7 is independent, generic infrastructure

**Decision:** `OwningPtr<T>`, `SharedOwningPtr<T>`, and `FileHandle`
(Phase 7) have no dependency on `Component`/`Gate`/`Netlist`/etc. — new
library `eda_memory`. The frontend demo additionally links `eda_oop` to
have `Component` as something concrete to own, which does not make the
library itself depend on it (same relationship any program using
`std::unique_ptr<MyClass>` has to the standard library).

**Why:** a smart pointer or RAII wrapper is domain-agnostic by nature —
there's no "new EDA capability" being built here, just infrastructure
that could own anything. Same category as Phase 4 and Phase 6:
independent, not because of a same-problem comparison this time, but
because there's nothing *to* depend on — the classes don't know or
care what they point at.

**How to apply:** future phases introducing generic infrastructure
(a thread pool in Phase 8, say) should default to independent unless
there's a concrete reason to couple it to a specific earlier phase's
type.

---

### 2026-10-18 — `SharedOwningPtr` has no converting constructor (a deliberate, documented gap)

**Decision:** `SharedOwningPtr<T>` does not support
`SharedOwningPtr<Base> p = makeShared<Derived>();` the way `OwningPtr<T>`
supports the equivalent for unique ownership. Kept simpler on purpose —
a correct converting constructor for shared ownership needs to handle
the control block correctly across the type change, which is more
machinery than this project's teaching goal here justifies.

**Hit directly, not just anticipated:** writing the Phase 7 demo with
`SharedOwningPtr<Component> first = makeShared<OrGate>();` failed to
compile with exactly the expected error (conversion from
`SharedOwningPtr<OrGate>` to `SharedOwningPtr<Component>`). Fixed by
constructing directly (`SharedOwningPtr<Component> first(new OrGate());`)
with a comment explaining why, rather than quietly adding the missing
feature to make the original line compile.

**How to apply:** if a future phase genuinely needs
`SharedOwningPtr<Base>` constructed from a `Derived` factory call,
that's the point to actually add the converting constructor (mirroring
`OwningPtr`'s), not before — avoid building capability ahead of a real
need.

---

### 2026-10-18 — ParallelSimulator depends on Component and ThreadPool; neither depends on Netlist

**Decision:** Phase 8 splits into two libraries. `ThreadPool` is fully
independent (no dependency on anything EDA-specific) — generic
infrastructure, same category as Phase 7's memory classes.
`ParallelSimulator` depends on BOTH `ThreadPool` (this same phase) and
`eda_oop`'s `Component` (reused, like Phases 3/4/6). Neither depends on
Phase 3's `Netlist`, even though `ParallelSimulator` and `Netlist` both
ultimately answer "what is every wire's value."

**Why this isn't a Phase-4-vs-Phase-3 situation:** Phase 4 was a
parallel technique because it improved the SAME axis Phase 3 already
addressed (how much work gets done — incremental vs. full recompute).
Phase 8 improves a DIFFERENT axis entirely: given the same full amount
of work `Netlist` would do, run the independent parts of it
concurrently instead of serially. Comparable optimization *goal*
(faster/less wasteful simulation), different optimization *axis*
(throughput via parallelism, not reduced work via incrementality) — not
a rematch of Phase 3 vs. 4, and not natural to build as an extension of
either.

**How to apply:** when a new phase's improvement is along a genuinely
different axis than an existing alternative (even if the end goal
sounds similar — "make simulation faster"), it's fine for it to be its
own independent thing rather than forcing it to relate to either
existing option as a dependency or a direct competitor.

---

### 2026-10-18 — ThreadPool's synchronization discipline: lock on both sides of every state change

**Decision:** every mutation of `ThreadPool`'s `pendingTasks_` counter,
and every `condition_variable::notify_*` call tied to it, happens while
holding the same `mutex_` that `waitUntilIdle()` uses to check its
predicate. Chosen deliberately over the (slightly faster) alternative
of decrementing/notifying after releasing the lock.

**Why:** decrementing and notifying without the lock creates a
textbook "lost wakeup" race — a worker could finish, decrement the
counter to zero, and call `notify_all()` in the narrow window between
`waitUntilIdle()` checking the predicate (false) and actually starting
to wait, permanently losing that one-shot notification and hanging
forever. This wasn't observed as a bug (the current implementation was
written correctly from the start), but reasoned through explicitly
before writing the code, and the resulting discipline is documented
directly in `ThreadPool.cpp`'s top comment so a future edit doesn't
reintroduce it by "optimizing" the locking away.

**Verified, not just reasoned about:** ran both Phase 8 test binaries
20 consecutive times each specifically hunting for intermittent
hangs/wrong counts (a subtly-wrong synchronization design often passes
most runs and fails occasionally). Zero failures across 40 runs.

**How to apply:** any future concurrency code in this project should
default to "mutation + notification under the same lock" unless there's
a measured, specific reason to split them — the lost-wakeup risk from
splitting them is easy to introduce and easy to miss in a quick read.

---

### 2026-10-18 — Phase 9's GUI stack: Dear ImGui (fetched as source) + GLFW (pacman) + OpenGL (system)

**Decision:** Dear ImGui pulled in via CMake `FetchContent`, pinned to
`v1.93.0` (checked against GitHub's actual latest release via the API
at the time, not guessed from memory). GLFW installed via `pacman -S
mingw-w64-ucrt-x86_64-glfw`. OpenGL via the system's `opengl32` (ships
with the base MinGW toolchain, confirmed `libopengl32.a` present —
no extra install).

**Why ImGui specifically isn't a pacman package like GoogleTest was:**
checked first (`pacman -Ss imgui`) — only Python bindings exist for
this toolchain, no C++ library package. This isn't a gap; Dear ImGui is
explicitly designed to be compiled directly into the consuming
project's own build (no "ImGui.dll" exists to link against) —
`FetchContent` + compiling its `.cpp` files ourselves is the library's
own intended integration path, not a workaround for a missing package.

**Why GLFW specifically, not SDL2 or a native Win32 backend:** GLFW has
a proper CMake config package (`find_package(glfw3 CONFIG)`) and is
one of Dear ImGui's two most common/best-supported backend
combinations (the other being SDL2) — picked for that support level,
not for any technical advantage over SDL2 for this project's needs.

**How to apply:** `main.cpp` depends on Phase 3's `Netlist` (new GUI
capability built on an existing engine, not a parallel technique) —
reconfirms the established "new problem, old building block → depend"
rule one more time. The vendored `imgui` CMake target deliberately
does NOT get this project's `-Wall -Wextra -Wpedantic` flags — those
are for code this project owns.

---

### 2026-10-18 — Phase 9's UI itself has no GoogleTest coverage, by deliberate scope decision

**Decision:** only `WaveformBuffer` (pure logic, no ImGui/GLFW
dependency) gets a GoogleTest suite. The actual rendering —
`ImGui::Checkbox`/`PlotLines`/the GLFW event loop — has none.

**Why:** there is no practical, meaningful assertion to write against
"does this rendered frame look correct" without either a human looking
at it or a much heavier pixel-diffing test infrastructure this small
project doesn't warrant. Writing hollow tests that merely call ImGui
functions without checking anything real would be worse than no tests
— false confidence. Instead: verified actual *functional* correctness
(does the app launch, run its full lifecycle, and shut down cleanly
without crashing) via a `--max-frames=N` automated smoke-test mode,
confirmed exit code 0 after a real window + GL context + full ImGui
init/render/shutdown cycle. Visual correctness (does it *look* right)
was explicitly left for the user to confirm by actually running the
app — stated plainly rather than silently claimed.

**How to apply:** this is the template for any future
rendering/UI-heavy work in this project — separate the testable logic
into its own dependency-free piece (as `WaveformBuffer` was), give
*that* full coverage, and use an automated smoke-test affordance for
the rendering shell's basic liveness, while being explicit to the user
about what wasn't (and likely can't practically be) verified
automatically.
