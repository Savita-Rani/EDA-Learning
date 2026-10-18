#include <gtest/gtest.h>
#include <sstream>
#include <eda/LogicValue.h>

// ============================================================================
// test_logic_value.cpp — Phase 6 tests, part 1: LogicValue's own truth
// tables (independent of LogicGate<T>).
// ============================================================================

using namespace eda;

namespace {

// Independent oracle — written differently from LogicValue.cpp's
// implementation, same philosophy as every previous phase's tests.
LogicValue expectedAnd(LogicValue a, LogicValue b) {
    if (a == LogicValue::Zero || b == LogicValue::Zero) return LogicValue::Zero;
    bool bothOne = (a == LogicValue::One) && (b == LogicValue::One);
    return bothOne ? LogicValue::One : LogicValue::Unknown;
}

LogicValue expectedOr(LogicValue a, LogicValue b) {
    if (a == LogicValue::One || b == LogicValue::One) return LogicValue::One;
    bool bothZero = (a == LogicValue::Zero) && (b == LogicValue::Zero);
    return bothZero ? LogicValue::Zero : LogicValue::Unknown;
}

LogicValue expectedNot(LogicValue a) {
    if (a == LogicValue::Zero) return LogicValue::One;
    if (a == LogicValue::One) return LogicValue::Zero;
    return LogicValue::Unknown; // Unknown and HighZ both invert to Unknown
}

const LogicValue kAllValues[] = {LogicValue::Zero, LogicValue::One, LogicValue::Unknown, LogicValue::HighZ};

} // namespace

TEST(LogicValueTest, AndTruthTable) {
    for (LogicValue a : kAllValues) {
        for (LogicValue b : kAllValues) {
            EXPECT_EQ(logicalAnd(a, b), expectedAnd(a, b))
                << "AND(" << a << ", " << b << ")";
        }
    }
}

TEST(LogicValueTest, OrTruthTable) {
    for (LogicValue a : kAllValues) {
        for (LogicValue b : kAllValues) {
            EXPECT_EQ(logicalOr(a, b), expectedOr(a, b))
                << "OR(" << a << ", " << b << ")";
        }
    }
}

TEST(LogicValueTest, NotTruthTable) {
    for (LogicValue a : kAllValues) {
        EXPECT_EQ(logicalNot(a), expectedNot(a)) << "NOT(" << a << ")";
    }
}

TEST(LogicValueTest, ToCharMatchesConventionalSymbols) {
    EXPECT_EQ(toChar(LogicValue::Zero), '0');
    EXPECT_EQ(toChar(LogicValue::One), '1');
    EXPECT_EQ(toChar(LogicValue::Unknown), 'X');
    EXPECT_EQ(toChar(LogicValue::HighZ), 'Z');
}

TEST(LogicValueTest, StreamOperatorMatchesToChar) {
    std::ostringstream out;
    out << LogicValue::One;
    EXPECT_EQ(out.str(), "1");
}
