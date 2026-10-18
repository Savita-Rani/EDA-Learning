#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <eda/Signal.h>

using namespace eda;

TEST(SignalTest, EmitCallsEveryConnectedSlot) {
    Signal<int> signal;
    int sumA = 0;
    int sumB = 0;
    signal.connect([&sumA](int v) { sumA += v; });
    signal.connect([&sumB](int v) { sumB += v * 2; });

    signal.emit(5);

    EXPECT_EQ(sumA, 5);
    EXPECT_EQ(sumB, 10);
}

TEST(SignalTest, DisconnectStopsThatSlotWithoutAffectingOthers) {
    Signal<int> signal;
    int countA = 0;
    int countB = 0;
    std::size_t idA = signal.connect([&countA](int) { ++countA; });
    signal.connect([&countB](int) { ++countB; });

    signal.emit(1);
    signal.disconnect(idA);
    signal.emit(1);
    signal.emit(1);

    EXPECT_EQ(countA, 1); // only saw the first emit
    EXPECT_EQ(countB, 3); // saw all three
}

TEST(SignalTest, SlotCountReflectsConnectAndDisconnect) {
    Signal<bool> signal;
    EXPECT_EQ(signal.slotCount(), 0u);

    std::size_t id = signal.connect([](bool) {});
    EXPECT_EQ(signal.slotCount(), 1u);

    signal.disconnect(id);
    EXPECT_EQ(signal.slotCount(), 0u);
}

TEST(SignalTest, WorksWithMultipleArgumentsOfDifferentTypes) {
    Signal<std::string, int> signal;
    std::vector<std::string> seenNames;
    std::vector<int> seenIds;

    signal.connect([&](const std::string& name, int id) {
        seenNames.push_back(name);
        seenIds.push_back(id);
    });

    signal.emit("OUT", 3);
    signal.emit("n1", 1);

    ASSERT_EQ(seenNames.size(), 2u);
    EXPECT_EQ(seenNames[0], "OUT");
    EXPECT_EQ(seenIds[0], 3);
    EXPECT_EQ(seenNames[1], "n1");
    EXPECT_EQ(seenIds[1], 1);
}

TEST(SignalTest, EmitWithNoConnectedSlotsIsSafe) {
    Signal<int> signal;
    signal.emit(42); // must not crash
    SUCCEED();
}
