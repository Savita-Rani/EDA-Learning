#include <gtest/gtest.h>
#include <eda/Gate.h>

// ============================================================================
// test_gate.cpp — Phase 1 tests, now on GoogleTest.
//
// Pulled forward from the original plan (GoogleTest/Catch2 was meant to
// wait for Phase 11) because having a real framework answer "how do I
// test this" for Phase 1 beats hand-rolled asserts. Installed via MSYS2's
// pacman: `mingw-w64-ucrt-x86_64-gtest`, matching our ucrt64 g++ toolchain.
// `tests/CMakeLists.txt` finds it with find_package(GTest CONFIG) and
// registers every TEST() below as its own CTest entry via
// gtest_discover_tests — run `ctest` and you'll see GateTest.AND,
// GateTest.OR, etc. individually, not one lump test.
//
// Still links only against eda_core — tests exercise the backend's PUBLIC
// API, same as any frontend would.
//
// Still deliberately recomputes each expected truth table from scratch
// (expected() below) instead of calling eda::Gate::evaluate() a second
// time — a test that re-derives its answer from the code it's checking
// can't catch a logic bug in that code.
//
// Concepts introduced here:
//  - TEST(Suite, Name) { ... }   : GoogleTest macro that expands into a
//    subclass of ::testing::Test with this body as its test method, and
//    registers an instance of it at static-init time. `Suite` groups
//    related tests (shows up as "GateTest.AND" etc. in output).
//  - EXPECT_EQ / EXPECT_STREQ    : "soft" assertions — record a failure
//    and keep running the rest of the test (vs. ASSERT_EQ, which stops
//    the current test immediately). We use EXPECT here so one wrong
//    input combination doesn't hide the other three.
//  - Why EXPECT_STREQ and not EXPECT_EQ for name()? `Gate::name()`
//    returns `const char*`. EXPECT_EQ on two `const char*` compares
//    POINTERS, not string contents — a classic gotcha. EXPECT_STREQ
//    compares the actual characters.
//  - `<<` streamed onto EXPECT_EQ : appends a custom message to the
//    failure output only (no effect when the check passes).
//  - No `main()` here — linking GTest::gtest_main (see CMakeLists.txt)
//    supplies one that runs every registered TEST().
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
    return false; // unreachable — every GateType is handled above
}

void expectTruthTable(GateType type, const char* expectedName) {
    Gate gate(type);
    EXPECT_STREQ(gate.name(), expectedName);

    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            EXPECT_EQ(gate.evaluate(a, b), expected(type, a, b))
                << expectedName << "(" << a << ", " << b << ")";
        }
    }
}

} // namespace

TEST(GateTest, AND)  { expectTruthTable(GateType::AND,  "AND");  }
TEST(GateTest, OR)   { expectTruthTable(GateType::OR,   "OR");   }
TEST(GateTest, NOT)  { expectTruthTable(GateType::NOT,  "NOT");  }
TEST(GateTest, NAND) { expectTruthTable(GateType::NAND, "NAND"); }
TEST(GateTest, NOR)  { expectTruthTable(GateType::NOR,  "NOR");  }
TEST(GateTest, XOR)  { expectTruthTable(GateType::XOR,  "XOR");  }
