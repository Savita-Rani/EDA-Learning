#include "eda/NetlistLoader.h"

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// ============================================================================
// NetlistLoader.cpp — Phase 5 implementation.
// ============================================================================

namespace eda {

namespace {

std::unique_ptr<Component> makeComponent(const std::string& gateType) {
    // string -> factory function. A tiny, informal preview of the Factory
    // pattern that Phase 10 formalizes — here it's just enough to avoid a
    // switch/if-chain, not a named abstraction yet.
    static const std::unordered_map<std::string, std::function<std::unique_ptr<Component>()>> factory = {
        {"AND",  [] { return std::make_unique<AndGate>(); }},
        {"OR",   [] { return std::make_unique<OrGate>(); }},
        {"NOT",  [] { return std::make_unique<NotGate>(); }},
        {"NAND", [] { return std::make_unique<NandGate>(); }},
        {"NOR",  [] { return std::make_unique<NorGate>(); }},
        {"XOR",  [] { return std::make_unique<XorGate>(); }},
    };

    auto it = factory.find(gateType);
    if (it == factory.end()) {
        throw std::runtime_error("Unknown gate type: " + gateType);
    }
    return it->second();
}

std::size_t expectedInputCount(const std::string& gateType) {
    return gateType == "NOT" ? 1 : 2;
}

} // namespace

Netlist loadNetlist(std::istream& input) {
    Netlist netlist;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        std::istringstream tokens(line);
        std::string gateType;
        if (!(tokens >> gateType)) {
            continue; // blank line
        }
        if (gateType[0] == '#') {
            continue; // comment
        }

        std::string outputWire;
        if (!(tokens >> outputWire)) {
            throw std::runtime_error("Line " + std::to_string(lineNumber) + ": missing output wire");
        }

        std::vector<std::string> inputWires;
        std::string wire;
        while (tokens >> wire) {
            inputWires.push_back(wire);
        }

        std::size_t expected = expectedInputCount(gateType);
        if (inputWires.size() != expected) {
            throw std::runtime_error(
                "Line " + std::to_string(lineNumber) + ": " + gateType + " expects " +
                std::to_string(expected) + " input wire(s), got " + std::to_string(inputWires.size()));
        }

        netlist.addComponent(makeComponent(gateType), std::move(inputWires), std::move(outputWire));
    }

    return netlist;
}

Netlist loadNetlistFile(const std::string& path) {
    std::ifstream file(path);
    // is_open(), not `!file`/fail() — on this project's g++/MinGW
    // (ucrt64) toolchain, constructing an ifstream from a path that
    // fails to open does NOT reliably set failbit (confirmed with an
    // isolated repro: is_open() correctly reports false, but !file,
    // fail(), bad(), and eof() all stay false too). is_open() is the
    // semantically correct check for "did opening this file succeed"
    // regardless, so prefer it over relying on stream-state operators.
    if (!file.is_open()) {
        throw std::runtime_error("Could not open netlist file: " + path);
    }
    return loadNetlist(file);
}

} // namespace eda
