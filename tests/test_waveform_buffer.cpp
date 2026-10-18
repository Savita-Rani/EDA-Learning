#include <gtest/gtest.h>
#include <eda/WaveformBuffer.h>

// ============================================================================
// test_waveform_buffer.cpp — Phase 9 tests.
//
// The only GoogleTest coverage for this phase — see
// docs/phases/phase9.md for why the ImGui/GLFW integration itself has
// none (and what was done instead to verify it works).
// ============================================================================

using namespace eda;

TEST(WaveformBufferTest, StartsEmpty) {
    WaveformBuffer buffer(5);
    EXPECT_EQ(buffer.size(), 0u);
    EXPECT_EQ(buffer.capacity(), 5u);
    EXPECT_TRUE(buffer.samples().empty());
}

TEST(WaveformBufferTest, PushesUpToCapacityWithoutDropping) {
    WaveformBuffer buffer(3);
    buffer.push(1.0f);
    buffer.push(2.0f);
    buffer.push(3.0f);

    ASSERT_EQ(buffer.size(), 3u);
    EXPECT_FLOAT_EQ(buffer.samples()[0], 1.0f);
    EXPECT_FLOAT_EQ(buffer.samples()[1], 2.0f);
    EXPECT_FLOAT_EQ(buffer.samples()[2], 3.0f);
}

TEST(WaveformBufferTest, DropsOldestSampleWhenOverCapacity) {
    WaveformBuffer buffer(3);
    buffer.push(1.0f);
    buffer.push(2.0f);
    buffer.push(3.0f);
    buffer.push(4.0f); // 1.0f must be dropped now

    ASSERT_EQ(buffer.size(), 3u);
    EXPECT_FLOAT_EQ(buffer.samples()[0], 2.0f);
    EXPECT_FLOAT_EQ(buffer.samples()[1], 3.0f);
    EXPECT_FLOAT_EQ(buffer.samples()[2], 4.0f);
}

TEST(WaveformBufferTest, SizeNeverExceedsCapacityUnderManyPushes) {
    WaveformBuffer buffer(4);
    for (int i = 0; i < 100; ++i) {
        buffer.push(static_cast<float>(i));
    }

    EXPECT_EQ(buffer.size(), 4u);
    // The last 4 values pushed were 96, 97, 98, 99.
    EXPECT_FLOAT_EQ(buffer.samples()[0], 96.0f);
    EXPECT_FLOAT_EQ(buffer.samples()[3], 99.0f);
}
