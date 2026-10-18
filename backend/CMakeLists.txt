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
