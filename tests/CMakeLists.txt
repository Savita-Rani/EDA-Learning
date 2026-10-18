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

# --- Phase 4 (Event-driven engine) — separate target, everything above untouched -
# Tests EventSimulator (eda_event), independent of eda_netlist.

add_executable(eda_event_tests
    test_event_simulator.cpp
)

target_link_libraries(eda_event_tests PRIVATE eda_event GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_event_tests PRIVATE /W4)
else()
    target_compile_options(eda_event_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(eda_event_tests)

# --- Phase 5 (File I/O) — separate target, everything above untouched -----
# Tests NetlistLoader (eda_fileio), which depends on eda_netlist's Netlist.

add_executable(eda_fileio_tests
    test_netlist_loader.cpp
)

target_link_libraries(eda_fileio_tests PRIVATE eda_fileio GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_fileio_tests PRIVATE /W4)
else()
    target_compile_options(eda_fileio_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(eda_fileio_tests)

# --- Phase 6 (Templates) — separate target, everything above untouched ----
# Tests LogicValue + LogicGate<T> (eda_generic), independent of everything
# else — a parallel technique, not built on Component/Gate.

add_executable(eda_generic_tests
    test_logic_value.cpp
    test_logic_gate.cpp
)

target_link_libraries(eda_generic_tests PRIVATE eda_generic GTest::gtest_main)

if(MSVC)
    target_compile_options(eda_generic_tests PRIVATE /W4)
else()
    target_compile_options(eda_generic_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

gtest_discover_tests(eda_generic_tests)
