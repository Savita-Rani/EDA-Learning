# tests — Phase 1 smoke test for eda_core, run via CTest.
# Links only against the backend, exactly like a frontend would: tests
# exercise eda_core's public API, never its private implementation.

add_executable(eda_tests
    test_gate.cpp
)

target_link_libraries(eda_tests PRIVATE eda_core)

if(MSVC)
    target_compile_options(eda_tests PRIVATE /W4)
else()
    target_compile_options(eda_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME GateTruthTables COMMAND eda_tests)
