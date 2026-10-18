# frontend/console_phase2 — Phase 2's UI: same console truth-table demo as
# Phase 1, rebuilt on the polymorphic Component hierarchy (eda_oop) instead
# of Gate (eda_core). Entirely separate from frontend/console/, which is
# left untouched.

add_executable(eda_console_phase2
    main.cpp
)

target_link_libraries(eda_console_phase2 PRIVATE eda_oop)

if(MSVC)
    target_compile_options(eda_console_phase2 PRIVATE /W4)
else()
    target_compile_options(eda_console_phase2 PRIVATE -Wall -Wextra -Wpedantic)
endif()
