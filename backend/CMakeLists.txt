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
