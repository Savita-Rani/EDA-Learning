#include <cstdio>
#include <cstdlib>
#include <string>
#include <eda/Gate.h>

// ============================================================================
// test_gate.cpp — Phase 1 smoke test.
//
// Not a real test framework (that's Phase 11, with Catch2/GoogleTest) — just
// plain checks, wired into CTest so `ctest` works from day one. Links only
// against eda_core, same as any frontend would: tests exercise the backend's
// PUBLIC API, nothing internal.
//
// Deliberately recomputes each expected truth table from scratch here
// instead of calling eda::Gate::evaluate() a second time — a test that
// re-derives its answer from the same code it's checking can't catch a
// logic bug in that code. If backend/src/Gate.cpp ever regresses (e.g. a
// typo turns NAND into AND during Phase 2's refactor), this test fails.
// ============================================================================

using namespace eda;

namespace {

bool expected(GateType type, bool a, bool b) {
    switch (type) {
        case GateType::AND:  return a && b;
        case GateType::OR:   return a || b;
        case GateType::NOT:  return !a;
        case GateType::NAND: return !(a && b);
        case GateType::NOR:  return !(a || b);
        case GateType::XOR:  return (a || b) && !(a && b); // written differently from Gate.cpp's `a != b` on purpose
    }
    std::abort(); // unreachable — every GateType is handled above
}

int checkCount = 0;
int failCount = 0;

void check(bool condition, const char* description) {
    ++checkCount;
    if (!condition) {
        ++failCount;
        std::fprintf(stderr, "FAIL: %s\n", description);
    }
}

void testGate(GateType type, const char* expectedName) {
    Gate gate(type);
    check(std::string(gate.name()) == expectedName, expectedName);

    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            bool got = gate.evaluate(a, b);
            bool want = expected(type, a, b);
            char msg[128];
            std::snprintf(msg, sizeof(msg), "%s(%d,%d) == %d", expectedName, a, b, want);
            check(got == want, msg);
        }
    }
}

} // namespace

int main() {
    testGate(GateType::AND,  "AND");
    testGate(GateType::OR,   "OR");
    testGate(GateType::NOT,  "NOT");
    testGate(GateType::NAND, "NAND");
    testGate(GateType::NOR,  "NOR");
    testGate(GateType::XOR,  "XOR");

    std::printf("%d/%d checks passed\n", checkCount - failCount, checkCount);
    return failCount == 0 ? 0 : 1;
}
