cmake_minimum_required(VERSION 3.15)
project(EDA_Tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# All executables land in build/bin/, regardless of which frontend
# subdirectory produced them — keeps launch.json / running binaries simple.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

# backend/        -> eda_core: the simulation engine, zero UI knowledge.
# frontend/console -> today's UI. frontend/imgui, frontend/qt can be added
#                      later as siblings; neither backend/ nor this file's
#                      structure needs to change when that happens.
# tests/          -> eda_tests, run via `ctest`; links only against eda_core.
add_subdirectory(backend)
add_subdirectory(frontend/console)
add_subdirectory(tests)

# Phase 2 (OOP) — new sibling directory, frontend/console above is untouched.
add_subdirectory(frontend/console_phase2)

# Phase 3 (STL) — new sibling directory, everything above is untouched.
add_subdirectory(frontend/netlist_demo)
