cmake_minimum_required(VERSION 3.15)
project(EDA_Tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# All executables land in build/bin/, regardless of which frontend
# subdirectory produced them — keeps launch.json / running binaries simple.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

# --- Phase 11 (Testing & tooling) -----------------------------------------
# Both OFF by default — opt-in via -DEDA_ENABLE_COVERAGE=ON /
# -DEDA_ENABLE_SANITIZERS=ON. Applied globally (every target, including
# GoogleTest/ImGui) because gcov/sanitizers need instrumentation
# consistently across whatever links together, not just our own code —
# but we only ever REPORT coverage for backend/src/*.cpp, see
# scripts/coverage-report.ps1.
#
# Sanitizers: this project's PRIMARY toolchain (MinGW/ucrt64 g++) has no
# libasan/libubsan at all — -fsanitize=... fails at the link step there
# (checked directly; see docs/phases/phase11.md). MSVC (already
# installed on this machine — VS2022 Community, C++ workload) DOES ship
# a working AddressSanitizer — VERIFIED end to end via
# scripts/sanitize-msvc.ps1: all 90 tests pass under it, and a
# standalone heap-buffer-overflow repro confirmed ASan actually catches
# real bugs (full report in docs/phases/phase11.md). Two environment
# quirks that script works around (the ASan runtime DLL not being on
# PATH by default, and an "interception failure" specific to this
# machine's environment) are also documented there. Note MSVC's
# sanitizer support is ASan ONLY — no UBSan equivalent; GCC/Clang's
# -fsanitize=undefined still only actually links on a toolchain that
# ships libubsan (not this project's MinGW one — covered by the
# ubuntu-latest CI job instead). EDA_ENABLE_COVERAGE's --coverage flag
# is GCC/Clang-specific and not meaningful under MSVC.

option(EDA_ENABLE_COVERAGE "Build with --coverage (gcov) instrumentation" OFF)
option(EDA_ENABLE_SANITIZERS "Build with AddressSanitizer (+ UBSan where available)" OFF)

# --- Phase 12 (Performance) ------------------------------------------------
# OFF by default, same convention as the two options above. -pg (gprof
# instrumentation) is GCC/Clang-specific, like --coverage — not
# meaningful under MSVC, so this one simply does nothing there rather
# than branching to an MSVC equivalent (Visual Studio's own profiler is
# a GUI tool, not a drop-in compiler flag; not wired up here since the
# profiling that motivated Phase 12 was done on the primary MinGW
# toolchain — see docs/phases/phase12.md).
option(EDA_ENABLE_PROFILING "Build with -pg (gprof) instrumentation" OFF)

if(EDA_ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

if(EDA_ENABLE_PROFILING AND NOT MSVC)
    add_compile_options(-pg -O2)
    add_link_options(-pg)
endif()

if(EDA_ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC AddressSanitizer (VS2019 16.9+). ASan only — no UBSan.
        # The runtime (clang_rt.asan_dynamic-*.dll) ships next to cl.exe
        # and must be reachable on PATH when running instrumented binaries.
        add_compile_options(/fsanitize=address /Zi)
    else()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# backend/        -> eda_core: the simulation engine, zero UI knowledge.
# frontend/console -> today's UI. frontend/imgui, frontend/qt can be added
#                      later as siblings; neither backend/ nor this file's
#                      structure needs to change when that happens.
# tests/          -> eda_tests, run via `ctest`; links only against eda_core.
add_subdirectory(backend)
add_subdirectory(tests)

# benchmarks/ (Phase 12) — only depends on backend libraries (eda_netlist,
# eda_fast_netlist), no GoogleTest/GLFW, so this builds under both the
# primary MinGW toolchain and MSVC, unlike frontend/* below.
add_subdirectory(benchmarks)

# frontend/* — GLFW/GoogleTest here were installed via pacman for the
# MinGW ABI; they're not binary-compatible with MSVC (different C++
# ABI/CRT entirely). None of this project's actual tests need any
# frontend/ demo, so under MSVC (used only for the AddressSanitizer
# path — see EDA_ENABLE_SANITIZERS above) these are skipped rather than
# attempting an MSVC-compatible GLFW/ImGui port, which nothing here
# currently needs. Under the normal MinGW toolchain, nothing changes —
# all frontends still build exactly as before.
if(NOT MSVC)
    add_subdirectory(frontend/console)

    # Phase 2 (OOP) — new sibling directory, frontend/console above is untouched.
    add_subdirectory(frontend/console_phase2)

    # Phase 3 (STL) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/netlist_demo)

    # Phase 4 (Event-driven engine) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/event_demo)

    # Phase 5 (File I/O) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/fileio_demo)

    # Phase 6 (Templates) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/generic_demo)

    # Phase 7 (Memory management) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/memory_demo)

    # Phase 8 (Concurrency) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/parallel_demo)

    # Phase 9 (Frontend, ImGui) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/imgui_demo)

    # Phase 10 (Design patterns) — new sibling directory, everything above is untouched.
    add_subdirectory(frontend/patterns_demo)
endif()
