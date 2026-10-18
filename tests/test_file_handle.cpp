#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <eda/FileHandle.h>

// ============================================================================
// test_file_handle.cpp — Phase 7 tests, part 3.
// ============================================================================

using namespace eda;

namespace {

std::string readWholeFile(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

} // namespace

TEST(FileHandleTest, WriteAndReadBackAfterScopeExit) {
    const std::string path = "phase7_test_write.log";
    {
        FileHandle handle(path);
        ASSERT_TRUE(handle.isOpen());
        handle.writeLine("first");
        handle.writeLine("second");
    } // destructor runs here — must flush and close

    EXPECT_EQ(readWholeFile(path), "first\nsecond\n");
    std::remove(path.c_str());
}

TEST(FileHandleTest, MoveTransfersTheOpenFile) {
    const std::string path = "phase7_test_move.log";
    {
        FileHandle a(path);
        ASSERT_TRUE(a.isOpen());

        FileHandle b = std::move(a);
        EXPECT_FALSE(a.isOpen());
        EXPECT_TRUE(b.isOpen());

        b.writeLine("via moved-to handle");
    }

    EXPECT_EQ(readWholeFile(path), "via moved-to handle\n");
    std::remove(path.c_str());
}

TEST(FileHandleTest, WriteAfterMoveFromIsSafeNoOp) {
    const std::string path = "phase7_test_noop.log";
    {
        FileHandle a(path);
        FileHandle b = std::move(a);
        (void)b;

        a.writeLine("should be silently ignored, not a crash");
        EXPECT_FALSE(a.isOpen());
    }
    std::remove(path.c_str());
}

TEST(FileHandleTest, OpeningAnInvalidPathFailsGracefully) {
    FileHandle handle("Z:/this/directory/does/not/exist/at/all/file.log");
    EXPECT_FALSE(handle.isOpen());
    handle.writeLine("must not crash even though nothing is open");
}
