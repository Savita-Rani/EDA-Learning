#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <eda/NetlistLoader.h>

// ============================================================================
// test_netlist_loader.cpp — Phase 5 tests.
//
// Links eda_fileio (transitively eda_netlist, eda_oop). Most tests parse
// from an in-memory std::istringstream — no filesystem needed — except
// LoadsRealFileFromDisk, which specifically exercises the
// open-a-real-file path in loadNetlistFile().
// ============================================================================

using namespace eda;

namespace {

bool expectedXor(bool a, bool b) {
    return (a || b) && !(a && b);
}

} // namespace

TEST(NetlistLoaderTest, ParsesSimpleAndGate) {
    std::istringstream stream("AND OUT A B\n");
    Netlist netlist = loadNetlist(stream);

    netlist.setInput("A", true);
    netlist.setInput("B", true);
    netlist.simulate();
    EXPECT_TRUE(netlist.wireValue("OUT"));
}

TEST(NetlistLoaderTest, ParsesFourNandXorAndSimulatesCorrectly) {
    const std::string circuit =
        "NAND n1 A B\n"
        "NAND n2 A n1\n"
        "NAND n3 B n1\n"
        "NAND OUT n2 n3\n";

    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            std::istringstream stream(circuit);
            Netlist netlist = loadNetlist(stream);

            netlist.setInput("A", a);
            netlist.setInput("B", b);
            netlist.simulate();

            EXPECT_EQ(netlist.wireValue("OUT"), expectedXor(a, b))
                << "XOR(" << a << ", " << b << ")";
        }
    }
}

TEST(NetlistLoaderTest, SkipsBlankLinesAndComments) {
    std::istringstream stream(
        "# a comment line\n"
        "\n"
        "AND OUT A B\n"
        "\n"
        "# trailing comment\n");

    Netlist netlist = loadNetlist(stream);
    netlist.setInput("A", true);
    netlist.setInput("B", true);
    netlist.simulate();
    EXPECT_TRUE(netlist.wireValue("OUT"));
}

TEST(NetlistLoaderTest, ThrowsOnUnknownGateType) {
    std::istringstream stream("FOO OUT A B\n");
    EXPECT_THROW(loadNetlist(stream), std::runtime_error);
}

TEST(NetlistLoaderTest, ThrowsOnWrongInputCount) {
    std::istringstream tooFew("AND OUT A\n");   // AND needs 2 inputs, got 1
    EXPECT_THROW(loadNetlist(tooFew), std::runtime_error);

    std::istringstream tooMany("NOT OUT A B\n"); // NOT needs 1 input, got 2
    EXPECT_THROW(loadNetlist(tooMany), std::runtime_error);
}

TEST(NetlistLoaderTest, ThrowsOnMissingFile) {
    EXPECT_THROW(loadNetlistFile("this_file_does_not_exist_12345.nl"), std::runtime_error);
}

TEST(NetlistLoaderTest, LoadsRealFileFromDisk) {
    const std::string path = "phase5_test_temp.nl";
    {
        std::ofstream out(path);
        out << "AND OUT A B\n";
    }

    Netlist netlist = loadNetlistFile(path);
    netlist.setInput("A", true);
    netlist.setInput("B", true);
    netlist.simulate();
    EXPECT_TRUE(netlist.wireValue("OUT"));

    std::remove(path.c_str());
}
