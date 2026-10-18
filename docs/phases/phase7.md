# Phase 7 — Memory management

**Goal:** formalize ownership rules that have been implicit since Phase
2's informal `std::unique_ptr` usage. Write a minimal unique-ownership
smart pointer (`OwningPtr<T>`) and a minimal shared-ownership one
(`SharedOwningPtr<T>`) by hand — not to replace `std::unique_ptr`/
`std::shared_ptr` (keep using those in real code), but because writing
out the Rule of 5 explicitly makes every rule visible instead of "the
compiler probably does the right thing." A third class, `FileHandle`,
applies the same RAII shape to a non-memory resource (an OS file handle)
to show the pattern is general.

**Status:** done. Builds clean, demo output correct (including a real
exception-safety demonstration), GoogleTest suite green (17/17 new,
55/55 total across all seven phases). No library-logic bug found this
time (the one compile error hit was a demo-usage mismatch with a known,
documented design limitation — see below — not a defect in
`OwningPtr`/`SharedOwningPtr`/`FileHandle` themselves).

## Relationship to earlier phases

`OwningPtr<T>`, `SharedOwningPtr<T>`, and `FileHandle` are fully
**independent**, generic infrastructure — no dependency on
`Component`/`Gate`/`Netlist`/etc., same category as Phase 4 and Phase 6
(there's nothing to "build on"; a smart pointer doesn't depend on what
it happens to point at). New library `eda_memory`.

The **frontend demo** additionally links `eda_oop` to have `Component`
as something concrete to own — this does **not** make the library
itself depend on `eda_oop`; it's the same relationship a real program
using `std::unique_ptr<MyClass>` has to the standard library (the smart
pointer doesn't know or care what `MyClass` is). See
`docs/DECISIONS.md`, "Phase 7 is independent, generic infrastructure."

Nothing in any earlier phase's files was touched. The three edited
`CMakeLists.txt` got 46 insertions, 0 deletions.

## Files

- `backend/include/eda/OwningPtr.h` — header-only (template; see "Why
  header-only," below). Unique ownership, explicit Rule of 5, a
  converting move constructor (`OwningPtr<Base> p = makeOwning<Derived>();`).
- `backend/include/eda/SharedOwningPtr.h` — header-only. Shared
  ownership via a simple reference count, assignment via the
  copy-and-swap idiom (deliberately different from `OwningPtr`'s style,
  for contrast). No converting constructor (kept simpler on purpose).
- `backend/include/eda/FileHandle.h` / `.cpp` — RAII around a C `FILE*`.
  Not a template — compiled into `eda_memory` normally.
- `frontend/memory_demo/main.cpp` + `CMakeLists.txt` — three
  demonstrations: unique ownership, shared ownership, and RAII surviving
  a thrown exception.
- `tests/test_owning_ptr.cpp`, `test_shared_owning_ptr.cpp`,
  `test_file_handle.cpp` — 17 GoogleTest cases, new `eda_memory_tests`
  target (3 source files, 1 executable).

## Why header-only (again)

Same reason as Phase 6's `LogicGate<T>`: `OwningPtr<T>`/
`SharedOwningPtr<T>` are class templates, and the compiler needs their
complete definition at every instantiation site. `FileHandle` is
**not** a template (`FILE*` isn't generic the way `T*` is), so it keeps
the ordinary `.h`/`.cpp` split every earlier phase used.

## The Rule of 5, written out twice, two different ways

**`OwningPtr<T>`** — the "traditional" style: separately-declared move
constructor and move assignment operator, each written out by hand:

```cpp
OwningPtr(OwningPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }
OwningPtr& operator=(OwningPtr&& other) noexcept {
    if (this != &other) { delete ptr_; ptr_ = other.ptr_; other.ptr_ = nullptr; }
    return *this;
}
```

**`SharedOwningPtr<T>`** — the **copy-and-swap idiom**: one
`operator=` taking its argument *by value*, handling both copy-assign
and move-assign through the same body:

```cpp
SharedOwningPtr& operator=(SharedOwningPtr other) noexcept {
    swap(other);
    return *this;
}
```

For `a = b`, the parameter `other` is **copy**-constructed from `b`
(incrementing the shared count); for `a = std::move(b)`, it's
**move**-constructed (not incrementing). Either way, `swap` exchanges
`*this`'s old state into `other`, which then gets destroyed (releasing
whatever `*this` used to own) when the function returns. One body,
both operations, and self-assignment (`a = a`) is handled correctly
*without any special-casing* — confirmed directly by
`SelfAssignmentIsSafe` in the tests.

## A design limitation, hit directly and documented, not just mentioned

`SharedOwningPtr` deliberately has **no converting constructor** (unlike
`OwningPtr`, see below) — kept simpler on purpose. This wasn't
theoretical: writing the demo as
`SharedOwningPtr<Component> first = makeShared<OrGate>();` **failed to
compile** —

```
error: conversion from 'SharedOwningPtr<eda::OrGate>' to non-scalar
type 'SharedOwningPtr<eda::Component>' requested
```

— fixed by constructing directly instead:
`SharedOwningPtr<Component> first(new OrGate());`. Left in the demo
with a comment explaining why, rather than quietly adding the missing
constructor — the scope decision (keep `SharedOwningPtr` simpler than
`OwningPtr`) is worth seeing hit a real wall, not just stated.

`OwningPtr<T>` *does* have this (a templated converting move
constructor, letting `OwningPtr<Base> p = makeOwning<Derived>();` work,
the same way real `std::unique_ptr` does):

```cpp
template <typename U>
OwningPtr(OwningPtr<U>&& other) noexcept : ptr_(other.release()) {}
```

`other.release()` yields a `U*`; this only compiles if `U*` converts to
`T*` (e.g. derived-to-base) — the language's own pointer-conversion
rules gate it, no extra type-trait machinery needed. Confirmed via
`ConvertingMoveConstructorFromDerivedToBase` in the tests, and used for
real in the demo (`makeOwning<AndGate>()` assigned to
`OwningPtr<Component>`).

## RAII surviving an exception — the actual payoff, demonstrated for real

`demonstrateRaiiSurvivesExceptions()` in the demo opens a `FileHandle`,
writes two lines, then **throws partway through** (before writing a
third). The exception is caught in `main`, and the file is reopened and
printed — showing both earlier lines present, correctly flushed and
closed, despite the function exiting via an exception rather than a
normal `return`:

```
--- RAII survives exceptions ---
caught: simulated failure partway through a run
file contents, read back after the exception:
n1 = AND(1, 1) = 1
n2 = OR(1, 0) = 1
```

This is the actual reason RAII matters more than "it's less typing than
manual cleanup": **no code path** — normal return, early return,
exception, `break`/`continue` out of a loop — can skip a destructor
call for an object whose scope has ended. A manually-`fclose()`'d file
handle would have leaked (left genuinely unclosed/unflushed) if the
`fclose()` call were written after the line that threw, with no
`try`/`finally` wrapping it (C++ has no `finally` — RAII is how the
language covers that need).

## How to build

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

New targets: `eda_memory` (library — only `FileHandle.cpp` compiles;
the two smart-pointer headers need no separate compilation),
`eda_memory_demo` (frontend), `eda_memory_tests` (tests, 3 source files
in 1 executable).

## How to run

```powershell
./build/bin/eda_memory_demo.exe
```

Note: this writes `phase7_trace.log` into the current working
directory (gitignored — see `.gitignore`'s `*.log` entry, added this
phase specifically because of this). Not a bug; it's the demo actually
exercising real file I/O, same as Phase 5's demo circuits are real files
on disk.

## How to test

```powershell
cd build
ctest --output-on-failure
```

17 new tests, 55 total across all phases. Highlights:

| Test | What it checks |
|---|---|
| `OwningPtrTest.MoveAssignmentDestroysPreviousOwnedObject` | moving into an `OwningPtr` that already owns something deletes the old object first — not a leak |
| `OwningPtrTest.ConvertingMoveConstructorFromDerivedToBase` | the `OwningPtr<Base> p = makeOwning<Derived>();` pattern actually works |
| `SharedOwningPtrTest.ObjectSurvivesUntilLastCopyIsDestroyed` / `ObjectDeletedWhenLastCopyGoes` | reference counting's actual guarantee: alive as long as ANY owner remains, deleted when the LAST one goes |
| `SharedOwningPtrTest.MoveDoesNotIncrementUseCount` | the copy-vs-move distinction made concrete via an observable count |
| `SharedOwningPtrTest.SelfAssignmentIsSafe` | `a = a` doesn't crash or corrupt state — copy-and-swap's free win |
| `FileHandleTest.MoveTransfersTheOpenFile` | the *actual open file*, not just a copy of the pointer, moves to the new owner |
| `FileHandleTest.WriteAfterMoveFromIsSafeNoOp` | using a moved-from handle doesn't crash (defensive, not undefined behavior) |

**Verified beyond the test suite:** compiled a standalone attempt to
copy an `OwningPtr<int>` and confirmed the actual diagnostic —
`error: use of deleted function 'eda::OwningPtr<T>::OwningPtr(const
eda::OwningPtr<T>&)'` — rather than just asserting the copy constructor
is deleted.

## Troubleshooting

See `docs/phases/phase1.md`'s troubleshooting section for general
build/CMake issues. Specific to this phase:

- **`SharedOwningPtr<Base> p = makeShared<Derived>();` fails to
  compile** — expected; `SharedOwningPtr` has no converting constructor
  (see "A design limitation" above). Construct directly:
  `SharedOwningPtr<Base> p(new Derived(...));`.
- **Leftover `phase7_trace.log` after running the demo** — harmless,
  gitignored; delete it freely.

## Concepts covered

| Concept | Where | Why it matters |
|---|---|---|
| RAII | all three classes | Resource acquired in the constructor, released in the destructor — guaranteed to happen regardless of how the scope ends, demonstrated directly via the exception-survival test. |
| Rule of 5, two idioms | `OwningPtr` (hand-written move ops) vs. `SharedOwningPtr` (copy-and-swap) | Two different, both-correct ways to satisfy the same 5-special-member contract. |
| `= delete` | `OwningPtr`, `FileHandle` copy operations | A compile-time-enforced "this operation makes no sense for this type," not just an unwritten convention. |
| Reference counting | `SharedOwningPtr` | A heap-allocated shared count; copy increments, destruction decrements, zero triggers the real delete. |
| Copy-and-swap idiom | `SharedOwningPtr::operator=` | By-value parameter + `swap` — one function body correctly handles copy-assign, move-assign, *and* self-assignment. |
| Templated converting constructor | `OwningPtr(OwningPtr<U>&&)` | Enables `OwningPtr<Base> = makeOwning<Derived>();`, gated by ordinary pointer-conversion rules, no extra machinery. |
| Perfect forwarding | `makeOwning`/`makeShared` | `Args&&...` + `std::forward` — the same mechanism `std::make_unique`/`std::make_shared` use internally. |
| `noexcept` on move operations | both smart pointers | A real precondition for some standard-library optimizations (e.g. `std::vector` moving instead of copying elements on reallocation) — stated here even though not exercised directly. |

## Next up

**Phase 8 — Concurrency:** simulating a large netlist is still
single-threaded — independent parts of a circuit evaluate serially even
though nothing requires that. Phase 8 introduces `std::thread`,
mutexes, and atomics to parallelize independent work. See
`docs/ROADMAP.md` → "Why this order" for the full chain.
