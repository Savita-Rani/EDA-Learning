# Phase 1 — Gate basics

**Goal:** represent a logic gate as a C++ class and print its truth table
from the console. No inheritance yet, no file I/O, no UI — just enough to
anchor the first batch of fundamentals in real, runnable code.

## Files

- `backend/include/eda/Gate.h` — declares `GateType` (`enum class`) and
  `Gate` (class), inside `namespace eda`. This is the backend's public API.
- `backend/src/Gate.cpp` — implements `Gate`'s constructor and
  `evaluate()`/`name()`.
- `frontend/console/main.cpp` — builds one `Gate` per type, prints each
  truth table. Links against `eda_core`; includes only `<eda/Gate.h>`.

(Originally these lived under flat `include/`/`src/`; restructured into
`backend/`/`frontend/console/` immediately after — see `docs/DECISIONS.md`,
"Backend/frontend separation" — to make the separation a build-level
guarantee instead of a convention, in time for a later Qt port if wanted.)

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
