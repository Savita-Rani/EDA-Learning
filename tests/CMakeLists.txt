# tests — Phase 1 tests for eda_core, on GoogleTest.
# Links only against the backend, exactly like a frontend would: tests
# exercise eda_core's public API, never its private implementation.

find_package(GTest CONFIG REQUIRED)

add_executable(eda_tests
    test_gate.cpp
)

target_link_libraries(eda_tests PRIVATE eda_core GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_tests PRIVATE /W4)
else()
    target_compile_options(eda_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Registers every TEST() in eda_tests as its own CTest entry (e.g.
# GateTest.AND, GateTest.OR, ...) by running the binary with
# --gtest_list_tests at configure/build time, instead of one lump
# add_test() entry for the whole executable.
include(GoogleTest)
gtest_discover_tests(eda_tests)
