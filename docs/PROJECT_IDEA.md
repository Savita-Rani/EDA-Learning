# Project Idea

One document to read first, before `README.md`'s build instructions or
`ROADMAP.md`'s phase-by-phase detail. If you only read one file to
understand *why this project is shaped the way it is*, read this one.

## The idea in one paragraph

Build a digital logic simulator — an EDA (Electronic Design Automation)
tool — not because the tool itself is the goal, but as a vehicle for
learning C++ from fundamentals through advanced/systems-level concepts.
Every phase of the project introduces new C++ techniques *in service of*
a real feature the simulator needs next, so the learning is never
abstract — each concept earns its place by fixing a concrete limitation
in what was already built.

## What "EDA tool" means here (and what it deliberately doesn't)

EDA covers a wide range of tools used to design electronics — from
circuit simulation down to physical chip layout. This project picks one
specific, well-scoped slice:

- **In scope: digital logic simulation.** Given a circuit described as
  gates connected by wires, compute what every signal's value is, given
  some inputs.
- **Explicitly out of scope: physical layout ("place and route").** Real
  chip design also has to decide where gates physically sit on a die and
  how to route literal metal traces between them without crossings,
  under timing/area constraints — a separate, much more geometric
  problem. This project never touches that.
- **Also out of scope (for now): analog/SPICE-style simulation** —
  continuous voltages and currents, differential equations. This project
  only ever deals with discrete logic values (originally `bool`; from
  Phase 6 onward, optionally a richer 0/1/X/Z value).

Picking digital logic simulation specifically — over a schematic
*editor*, or an analog/SPICE simulator — was a deliberate choice early
on: it gives the best ratio of "teaches core C++ progressively" to
"visibly working demo almost immediately." (Logged in `docs/DECISIONS.md`,
2026-10-18, "EDA tool type.")

## The domain vocabulary, briefly

A few terms used throughout the code and docs, defined plainly:

- **Gate / component** — a unit that computes a boolean function of one
  or two inputs (AND, OR, NOT, NAND, NOR, XOR in this project).
- **Wire** — a named connection carrying one signal value. Not modeled
  as an object in this codebase — just a shared `std::string` name that
  one component's output and another's input both refer to. See the
  "wiring" explanation below.
- **Signal** — the value carried on a wire at a given moment (`bool`
  through Phase 5; optionally a 4-valued `LogicValue` — 0/1/X/Z — from
  Phase 6 onward).
- **Netlist** — a full circuit description: a set of components, each
  wired to specific input/output wire names. The word comes from real
  EDA tooling (a literal "list of nets," i.e. named wires).
- **Wiring** — the connections between gates: which gate's output feeds
  which other gate's input. In this project's code, two components are
  "wired together" purely because one's `outputWire` string matches
  another's `inputWires` string — the whole circuit graph emerges from
  that string matching, nothing more exotic. (Worked through with a full
  example — the 4-NAND XOR circuit — in the conversation that produced
  this doc; see `docs/phases/phase3.md` for the code that acts on it.)
- **Simulation** — computing every wire's value, given some inputs. This
  project explores two different techniques for this: recompute
  everything from scratch in topological order (Phase 3's `Netlist`), or
  propagate only actual changes via an event queue (Phase 4's
  `EventSimulator`).

## The learning philosophy: a causal chain, not a fixed syllabus

Each phase exists because the *previous* phase's design has exactly one
concrete limitation, and that phase's new C++ feature is the standard
tool for closing it. Difficulty tracks necessity, not an arbitrary
"basics → advanced" checklist imposed from outside. The full chain —
what every phase (including ones not yet built) fixes and what it
leaves open for the next — lives in `docs/ROADMAP.md` under "Why this
order."

Concretely, so far:

| Phase | Fixed | Left open (→ next phase) |
|---|---|---|
| 1. Basics | — | a `switch` means every new gate type needs code edited in two places |
| 2. OOP | new gate type = new class, zero edits elsewhere | gates are isolated — nothing connects them into a circuit |
| 3. STL | a netlist (graph) stores and auto-orders evaluation | every call recomputes the *entire* circuit from scratch |
| 4. Event-driven engine | only re-evaluates what actually changed | circuits are still hand-built in C++ code |
| 5. File I/O | circuits load from a text file (data, not code) | signals are hardcoded `bool` — no "unknown" state |
| 6. Templates | one generic gate works for `bool` *or* real 0/1/X/Z logic | no rigorous ownership rules for heap-allocated components yet |

## The architecture rule that shapes every file in this repo

**A later phase never edits an earlier phase's files.** Every phase's
code lives in new files; the previous phase's implementation, frontend,
and tests stay byte-for-byte untouched and keep building and passing
forever. This is why `backend/` has six separate static libraries
instead of one evolving one, and why there are six frontend executables
and six test binaries, all coexisting in a single build.

This isn't just a style preference — it's what makes the project useful
*as a learning artifact after the fact*: Phase 1's `switch`-based `Gate`
and Phase 2's polymorphic `Component` are both still there, runnable
side by side, so the actual difference between the two techniques is
something you can run and diff, not just something described in a
commit message.

Within that rule, two different relationships show up between phases —
distinguishing them matters:

- **Depend (reuse):** when a phase solves a genuinely *new* problem using
  an earlier phase's piece as a building block. Phase 3's `Netlist`
  depends on Phase 2's `Component`; Phase 5's `NetlistLoader` depends on
  Phase 3's `Netlist`.
- **Independent (parallel technique):** when a phase solves the *same*
  problem an earlier phase solved, just differently — kept independent
  specifically so there's something to compare. Phase 2's `Component` vs.
  Phase 1's `Gate`; Phase 4's `EventSimulator` vs. Phase 3's `Netlist`;
  Phase 6's `LogicGate<T>` vs. Phase 1/2's gates.

(Full reasoning and the exact rule for telling these apart:
`docs/DECISIONS.md`, "Phase isolation," and the follow-up entries after
Phases 3, 4, and 6.)

## Testing is part of the idea, not an afterthought

GoogleTest was deliberately pulled forward into Phase 1 rather than
waiting for the "Testing" phase (originally planned as Phase 11) — the
project treats "how do I verify this phase actually works" as inherent
to each phase, not a separate concern bolted on later. Tests
independently recompute their expected answers rather than re-deriving
them from the code under test, specifically so they can catch real
logic bugs rather than just mirroring them. This has already found two
genuine defects (a race condition in Phase 4's event batching, and a
toolchain-specific `ifstream` quirk in Phase 5) — not staged
demonstrations, actual bugs caught before they shipped.

## Where things stand, and where they're headed

Six phases done (basics → OOP → STL/graphs → event-driven simulation →
file I/O → templates/concepts), all coexisting, 38/38 tests passing. The
frontend is console-only so far — a real GUI (schematic + waveform view,
via Dear ImGui) arrives in Phase 9, once the simulation core itself is
done (Phases 7-8: rigorous memory ownership, then concurrency). Full
detail in `docs/ROADMAP.md`; day-to-day build/run/test instructions in
`README.md` and each phase's `docs/phases/phaseN.md`.
