#include <gtest/gtest.h>
#include <eda/BufferGate.h>

using namespace eda;

TEST(BufferGateTest, PassesInputThroughUnchanged) {
    BufferGate gate;
    EXPECT_STREQ(gate.name(), "BUFFER");
    EXPECT_FALSE(gate.evaluate(false));
    EXPECT_TRUE(gate.evaluate(true));
}

TEST(BufferGateTest, SecondInputIsIgnored) {
    BufferGate gate;
    EXPECT_EQ(gate.evaluate(true, false), gate.evaluate(true, true));
    EXPECT_EQ(gate.evaluate(false, false), gate.evaluate(false, true));
}
