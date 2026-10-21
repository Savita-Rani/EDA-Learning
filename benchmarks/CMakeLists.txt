# benchmarks — Phase 12 (Performance).
#
# Links eda_netlist and eda_fast_netlist directly, measuring real
# wall-clock time for repeated simulate() calls on the same circuit
# shape. No GoogleTest/GLFW dependency, so this builds under both the
# primary MinGW toolchain and MSVC. See docs/phases/phase12.md for the
# captured numbers and what they mean.

add_executable(eda_netlist_bench
    netlist_bench.cpp
)

target_link_libraries(eda_netlist_bench PRIVATE eda_netlist eda_fast_netlist)

if(MSVC)
    target_compile_options(eda_netlist_bench PRIVATE /W4)
else()
    target_compile_options(eda_netlist_bench PRIVATE -Wall -Wextra -Wpedantic)
endif()
