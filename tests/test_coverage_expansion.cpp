#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <eda/ComponentVisitor.h>
#include <eda/FileHandle.h>
#include <eda/NetlistLoader.h>

// ============================================================================
// test_coverage_expansion.cpp — Phase 11.
//
// Not new features — new TESTS, specifically closing real gaps found by
// running this project's full suite under gcov (--coverage) and reading
// the annotated output. All three gaps below belong to earlier phases'
// code; per this project's isolation rule, their original test files
// (tests/test_file_handle.cpp, test_component_visitor.cpp,
// test_netlist_loader.cpp) are NOT edited — these cases live here
// instead. See docs/phases/phase11.md for the full measured coverage
// numbers and which gaps were (and weren't) worth closing.
//
// Links eda_memory (FileHandle), eda_patterns (ComponentVisitor),
// eda_fileio (NetlistLoader) — three existing pieces, reused, same as
// any frontend demo linking multiple backend libraries at once.
// ============================================================================

using namespace eda;

// --- Gap 1: FileHandle::operator=(FileHandle&&) — move ASSIGNMENT was
// never exercised; Phase 7's own tests only covered move CONSTRUCTION. ---

TEST(CoverageExpansionTest, FileHandleMoveAssignmentClosesPreviousFileFirst) {
    const std::string pathA = "phase11_test_a.log";
    const std::string pathB = "phase11_test_b.log";

    {
        FileHandle a(pathA);
        FileHandle b(pathB);
        ASSERT_TRUE(a.isOpen());
        ASSERT_TRUE(b.isOpen());

        a.writeLine("original a content");
        b.writeLine("original b content");

        a = std::move(b); // a's original file must be closed here, not leaked
        ASSERT_TRUE(a.isOpen());
        a.writeLine("now writing through moved-in handle");
    }

    std::ifstream checkA(pathA);
    std::ostringstream contentsA;
    contentsA << checkA.rdbuf();
    // pathA's file was closed by the move-assignment before `a` took
    // over pathB's handle — its content (written before the move) is
    // exactly what was flushed at that point, and nothing more.
    EXPECT_EQ(contentsA.str(), "original a content\n");

    std::ifstream checkB(pathB);
    std::ostringstream contentsB;
    contentsB << checkB.rdbuf();
    EXPECT_EQ(contentsB.str(), "original b content\nnow writing through moved-in handle\n");

    std::remove(pathA.c_str());
    std::remove(pathB.c_str());
}

// --- Gap 2: ComponentVisitor's OR/NOT/NOR dispatch branches — Phase 10's
// own tests only exercised AND/NAND/XOR/BUFFER/unknown. ---

namespace {
struct RecordingVisitor : ComponentVisitor {
    std::string lastVisited;
    void visitAnd(const AndGate&) override { lastVisited = "AND"; }
    void visitOr(const OrGate&) override { lastVisited = "OR"; }
    void visitNot(const NotGate&) override { lastVisited = "NOT"; }
    void visitNand(const NandGate&) override { lastVisited = "NAND"; }
    void visitNor(const NorGate&) override { lastVisited = "NOR"; }
    void visitXor(const XorGate&) override { lastVisited = "XOR"; }
};
} // namespace

TEST(CoverageExpansionTest, VisitorDispatchesOrNotNorCorrectly) {
    RecordingVisitor visitor;

    acceptVisitor(OrGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "OR");

    acceptVisitor(NotGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "NOT");

    acceptVisitor(NorGate(), visitor);
    EXPECT_EQ(visitor.lastVisited, "NOR");
}

// --- Gap 3: NetlistLoader's OR/NOT/NOR/XOR factory branches, and the
// "missing output wire" parse error — Phase 5's own tests only covered
// AND/NAND gate types and a couple of unrelated error cases. ---

TEST(CoverageExpansionTest, NetlistLoaderParsesEveryGateTypeNotJustAndNand) {
    std::istringstream stream(
        "OR orOut A B\n"
        "NOT notOut A\n"
        "NOR norOut A B\n"
        "XOR xorOut A B\n");
    Netlist netlist = loadNetlist(stream);

    netlist.setInput("A", true);
    netlist.setInput("B", false);
    netlist.simulate();

    EXPECT_TRUE(netlist.wireValue("orOut"));    // OR(1,0)
    EXPECT_FALSE(netlist.wireValue("notOut"));  // NOT(1)
    EXPECT_FALSE(netlist.wireValue("norOut"));  // NOR(1,0)
    EXPECT_TRUE(netlist.wireValue("xorOut"));   // XOR(1,0)
}

TEST(CoverageExpansionTest, NetlistLoaderThrowsOnLineMissingOutputWire) {
    std::istringstream stream("AND\n"); // gate type present, nothing after it
    EXPECT_THROW(loadNetlist(stream), std::runtime_error);
}
