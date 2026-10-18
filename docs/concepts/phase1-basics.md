# Phase 1 — Gate basics

**Goal:** represent a logic gate as a C++ class and print its truth table
from the console. No inheritance yet, no file I/O, no UI — just enough to
anchor the first batch of fundamentals in real, runnable code.

## Files

- `include/Gate.h` — declares `GateType` (`enum class`) and `Gate` (class).
- `src/Gate.cpp` — implements `Gate`'s constructor and `evaluate()`/`name()`.
- `src/main.cpp` — builds one `Gate` per type, prints each truth table.

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
