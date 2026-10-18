#pragma once

#include <istream>
#include <string>

#include "eda/Netlist.h"

// ============================================================================
// NetlistLoader.h  —  Phase 5 (File I/O)
//
// Depends on Phase 3's Netlist (reused, not duplicated) — new capability
// (parsing a circuit description) built on an existing piece, the same
// relationship Phase 3 had to Phase 2's Component. Does not touch
// Netlist.h/.cpp, EventSimulator.h/.cpp, or anything earlier.
//
// Fixes Phase 4's leftover limitation: circuits were only ever hand-built
// via addComponent() calls in C++ source. This phase describes a circuit
// as DATA in a text file instead.
//
// File format — one component per line, whitespace-separated:
//   GATE_TYPE OUTPUT_WIRE INPUT_WIRE1 [INPUT_WIRE2]
// GATE_TYPE is one of AND/OR/NOT/NAND/NOR/XOR. NOT takes exactly one
// input wire; everything else takes exactly two. Blank lines and lines
// starting with '#' are ignored. External input VALUES are deliberately
// NOT part of this format — a netlist file describes circuit STRUCTURE
// only (which real EDA netlist formats also do); supplying input values
// is simulation stimulus, set via Netlist::setInput() same as before.
//
// Concepts introduced here:
//  - std::istream (not std::ifstream specifically) as the parsing
//    function's parameter type: lets tests parse from an in-memory
//    std::istringstream without touching the filesystem, while
//    loadNetlistFile() below is a thin convenience wrapper that opens a
//    real std::ifstream and forwards to the same logic. One parser, two
//    ways to feed it.
//  - Exceptions for malformed input: an unreadable file, an unknown gate
//    type, or the wrong number of input wires for a gate type all throw
//    std::runtime_error with a message pointing at the specific problem
//    (and line number, for in-file errors) — not a silent wrong netlist.
// ============================================================================

namespace eda {

// Parses a netlist description from an already-open stream and returns a
// fully-wired Netlist, ready for setInput()/simulate(). Throws
// std::runtime_error on a malformed line (unknown gate type, wrong
// input-wire count, missing output wire).
Netlist loadNetlist(std::istream& input);

// Convenience wrapper: opens `path` and parses it the same way. Throws
// std::runtime_error if the file can't be opened, in addition to every
// error loadNetlist(std::istream&) can throw.
Netlist loadNetlistFile(const std::string& path);

} // namespace eda
