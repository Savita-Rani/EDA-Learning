#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <eda/BufferGate.h>
#include <eda/ComponentFactory.h>

using namespace eda;

TEST(ComponentFactoryTest, StandardFactoryCreatesAllSixGatesCorrectly) {
    ComponentFactory factory = makeStandardGateFactory();

    EXPECT_TRUE(factory.create("AND")->evaluate(true, true));
    EXPECT_TRUE(factory.create("OR")->evaluate(false, true));
    EXPECT_TRUE(factory.create("NOT")->evaluate(false));
    EXPECT_FALSE(factory.create("NAND")->evaluate(true, true));
    EXPECT_FALSE(factory.create("NOR")->evaluate(false, true));
    EXPECT_TRUE(factory.create("XOR")->evaluate(true, false));
}

TEST(ComponentFactoryTest, CanRegisterANewTypeWithoutModifyingFactorySource) {
    ComponentFactory factory = makeStandardGateFactory();
    EXPECT_FALSE(factory.isRegistered("BUFFER"));

    factory.registerType("BUFFER", [] { return std::make_unique<BufferGate>(); });

    EXPECT_TRUE(factory.isRegistered("BUFFER"));
    auto gate = factory.create("BUFFER");
    EXPECT_STREQ(gate->name(), "BUFFER");
    EXPECT_TRUE(gate->evaluate(true));
}

TEST(ComponentFactoryTest, CreateThrowsForUnregisteredType) {
    ComponentFactory factory = makeStandardGateFactory();
    EXPECT_THROW(factory.create("UNKNOWN"), std::out_of_range);
}

TEST(ComponentFactoryTest, RegisterTypeThrowsOnDuplicateName) {
    ComponentFactory factory = makeStandardGateFactory();
    EXPECT_THROW(
        factory.registerType("AND", [] { return std::make_unique<AndGate>(); }),
        std::invalid_argument);
}

TEST(ComponentFactoryTest, FailedRegistrationDoesNotCorruptExistingState) {
    // Strong exception guarantee check: a rejected registerType() call
    // must leave the factory exactly as usable as before the attempt.
    ComponentFactory factory = makeStandardGateFactory();

    EXPECT_THROW(
        factory.registerType("AND", [] { return std::make_unique<OrGate>(); }),
        std::invalid_argument);

    // The ORIGINAL "AND" registration must still be intact and correct —
    // not overwritten, not removed, not left in some half-updated state.
    EXPECT_TRUE(factory.create("AND")->evaluate(true, true));
    EXPECT_FALSE(factory.create("AND")->evaluate(true, false));
}

TEST(ComponentFactoryTest, RegisteredTypesListsEveryRegisteredName) {
    ComponentFactory factory = makeStandardGateFactory();
    std::vector<std::string> types = factory.registeredTypes();

    EXPECT_EQ(types.size(), 6u);
    for (const char* expected : {"AND", "OR", "NOT", "NAND", "NOR", "XOR"}) {
        EXPECT_NE(std::find(types.begin(), types.end(), expected), types.end())
            << "missing: " << expected;
    }
}
