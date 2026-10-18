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

# --- Phase 2 (OOP) — separate target, eda_tests above is untouched --------
# Tests Component/AndGate/OrGate/... (eda_oop), not Gate (eda_core).

add_executable(eda_component_tests
    test_component.cpp
)

target_link_libraries(eda_component_tests PRIVATE eda_oop GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_component_tests PRIVATE /W4)
else()
    target_compile_options(eda_component_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(eda_component_tests)

# --- Phase 3 (STL) — separate target, everything above is untouched -------
# Tests Netlist (eda_netlist), which depends on eda_oop's Component.

add_executable(eda_netlist_tests
    test_netlist.cpp
)

target_link_libraries(eda_netlist_tests PRIVATE eda_netlist GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_netlist_tests PRIVATE /W4)
else()
    target_compile_options(eda_netlist_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(eda_netlist_tests)
