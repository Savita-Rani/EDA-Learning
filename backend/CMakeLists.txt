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

# --- Phase 5 (File I/O) — separate target, nothing above touched ---------
# NetlistLoader (eda/NetlistLoader.h) DEPENDS ON eda_netlist's Netlist —
# reused, not duplicated. New capability (parsing) built on an existing
# piece, same relationship Phase 3 had to Phase 2. See docs/DECISIONS.md,
# "Phase 5 depends on Phase 3."

add_library(eda_fileio STATIC
    src/NetlistLoader.cpp
)

target_include_directories(eda_fileio PUBLIC include)
target_link_libraries(eda_fileio PUBLIC eda_netlist)

if(MSVC)
    target_compile_options(eda_fileio PRIVATE /W4)
else()
    target_compile_options(eda_fileio PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 6 (Templates) — separate target, nothing above touched --------
# LogicGate<T> (eda/LogicGate.h) + LogicValue (eda/LogicValue.h) are a
# NEW, INDEPENDENT hierarchy — not built on Component/Gate, and don't
# touch them. Same gate behaviors, generalized via templates to work over
# more than one signal value type. A parallel technique, like Phase 4 was
# to Phase 3. See docs/DECISIONS.md, "Phase 6 is independent, like Phase
# 2 and 4." Only LogicValue.cpp needs compiling — LogicGate.h's template
# class is necessarily header-only (see the comment at its top for why).

add_library(eda_generic STATIC
    src/LogicValue.cpp
)

target_include_directories(eda_generic PUBLIC include)
target_compile_features(eda_generic PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(eda_generic PRIVATE /W4)
else()
    target_compile_options(eda_generic PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 7 (Memory management) — separate target, nothing above touched -
# OwningPtr<T>/SharedOwningPtr<T> (header-only templates) and FileHandle
# are NEW, INDEPENDENT, fully generic infrastructure — no dependency on
# Component/Gate/Netlist/etc. Only FileHandle.cpp needs compiling; the
# two smart-pointer templates are header-only by necessity (same reason
# as Phase 6's LogicGate<T>). The frontend demo separately links eda_oop
# too, to have a domain type (Component) to actually own — that does NOT
# make this library itself depend on eda_oop. See docs/DECISIONS.md,
# "Phase 7 is independent, generic infrastructure."

add_library(eda_memory STATIC
    src/FileHandle.cpp
)

target_include_directories(eda_memory PUBLIC include)
target_compile_features(eda_memory PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(eda_memory PRIVATE /W4)
else()
    target_compile_options(eda_memory PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 8 (Concurrency) — separate targets, nothing above touched -----
# ThreadPool is NEW, INDEPENDENT, fully generic infrastructure (no
# dependency on Component/Gate/etc.), same category as Phase 7's memory
# classes. ParallelSimulator DEPENDS ON both ThreadPool (this phase) and
# eda_oop's Component (reused, like Phases 3/4/6) — new capability
# (parallel evaluation) built on two existing pieces. See
# docs/DECISIONS.md, "ParallelSimulator depends on Component and
# ThreadPool."

find_package(Threads REQUIRED)

add_library(eda_threadpool STATIC
    src/ThreadPool.cpp
)

target_include_directories(eda_threadpool PUBLIC include)
target_link_libraries(eda_threadpool PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(eda_threadpool PRIVATE /W4)
else()
    target_compile_options(eda_threadpool PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eda_parallel STATIC
    src/ParallelSimulator.cpp
)

target_include_directories(eda_parallel PUBLIC include)
target_link_libraries(eda_parallel PUBLIC eda_oop eda_threadpool)

if(MSVC)
    target_compile_options(eda_parallel PRIVATE /W4)
else()
    target_compile_options(eda_parallel PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- Phase 9 (Frontend) — separate target, nothing above touched ---------
# WaveformBuffer (eda/WaveformBuffer.h) is independent, generic logic —
# the one piece of this phase that's testable with GoogleTest (the
# ImGui/GLFW rendering itself isn't; see docs/phases/phase9.md). No .cpp
# at all (every member is defined inline in the header), so this is an
# INTERFACE library, not STATIC — there is nothing to compile, only an
# include path to propagate to whatever links it.

add_library(eda_waveform INTERFACE)
target_include_directories(eda_waveform INTERFACE include)

# --- Phase 10 (Design patterns) — separate targets, nothing above touched -
# ComponentFactory/BufferGate/ComponentVisitor all depend on eda_oop's
# Component (reused, like Phases 3/4/6/8/9) — new capability (factory,
# traversal) built on an existing piece, grouped into one eda_patterns
# library since they share that dependency. Signal<Args...> is fully
# independent, generic infrastructure (no .cpp — header-only template,
# same reason as Phase 6/7/9's INTERFACE-shaped pieces) — kept in its
# own eda_signal INTERFACE library specifically so it does NOT carry an
# eda_oop dependency it doesn't need. See docs/DECISIONS.md, "Phase 10
# splits by dependency shape, same as Phases 8 and 9."

add_library(eda_patterns STATIC
    src/BufferGate.cpp
    src/ComponentFactory.cpp
    src/ComponentVisitor.cpp
)

target_include_directories(eda_patterns PUBLIC include)
target_link_libraries(eda_patterns PUBLIC eda_oop)

if(MSVC)
    target_compile_options(eda_patterns PRIVATE /W4)
else()
    target_compile_options(eda_patterns PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eda_signal INTERFACE)
target_include_directories(eda_signal INTERFACE include)

# --- Phase 12 (Performance) — separate target, nothing above touched -----
# FastNetlist (eda/FastNetlist.h) depends on eda_oop's Component (reused,
# like Phases 3/4/6/8/9/10) but is INDEPENDENT of eda_netlist — a second,
# parallel technique for the same problem Netlist solves, not an edit to
# it. See docs/DECISIONS.md, "Phase 12 is a parallel technique, not a
# Netlist edit," and docs/phases/phase12.md for the profiling that
# motivated it.

add_library(eda_fast_netlist STATIC
    src/FastNetlist.cpp
)

target_include_directories(eda_fast_netlist PUBLIC include)
target_link_libraries(eda_fast_netlist PUBLIC eda_oop)

if(MSVC)
    target_compile_options(eda_fast_netlist PRIVATE /W4)
else()
    target_compile_options(eda_fast_netlist PRIVATE -Wall -Wextra -Wpedantic)
endif()
