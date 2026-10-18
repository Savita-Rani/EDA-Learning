# backend — the simulation engine, built as a static library.
#
# PUBLIC include dir: anything that links against eda_core automatically
# gets backend/include on its include path, and can #include <eda/Gate.h>.
# This is the ONLY thing a frontend should ever need to know about us.

add_library(eda_core STATIC
    src/Gate.cpp
)

target_include_directories(eda_core PUBLIC include)

if(MSVC)
    target_compile_options(eda_core PRIVATE /W4)
else()
    target_compile_options(eda_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 2 (OOP) — separate target, eda_core above is untouched ---------
# Component/AndGate/OrGate/... (eda/Component.h) is a brand new hierarchy,
# not a modification of Gate. Kept as its own static library so Phase 1's
# eda_core build is unaffected by anything added here.

add_library(eda_oop STATIC
    src/Component.cpp
)

target_include_directories(eda_oop PUBLIC include)

if(MSVC)
    target_compile_options(eda_oop PRIVATE /W4)
else()
    target_compile_options(eda_oop PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 3 (STL) — separate target, eda_oop above is untouched ---------
# Netlist (eda/Netlist.h) DEPENDS ON eda_oop's Component — reused, not
# duplicated, not edited. Unlike Phase 2 (a parallel reimplementation of
# Phase 1 for comparison), Phase 3 is meant to build on Phase 2 directly;
# see docs/DECISIONS.md, "Phase 3 depends on Phase 2."

add_library(eda_netlist STATIC
    src/Netlist.cpp
)

target_include_directories(eda_netlist PUBLIC include)
target_link_libraries(eda_netlist PUBLIC eda_oop)

if(MSVC)
    target_compile_options(eda_netlist PRIVATE /W4)
else()
    target_compile_options(eda_netlist PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 4 (Event-driven engine) — separate target, nothing above touched -
# EventSimulator (eda/EventSimulator.h) depends on eda_oop's Component
# (reused, like Phase 3), but is INDEPENDENT of eda_netlist — it's a
# second, parallel technique for the same problem Netlist solves, not an
# extension of it. See docs/DECISIONS.md, "Phase 4 is a parallel
# technique, not a Netlist extension."

add_library(eda_event STATIC
    src/EventSimulator.cpp
)

target_include_directories(eda_event PUBLIC include)
target_link_libraries(eda_event PUBLIC eda_oop)

if(MSVC)
    target_compile_options(eda_event PRIVATE /W4)
else()
    target_compile_options(eda_event PRIVATE -Wall -Wextra -Wpedantic)
endif()
