# frontend/patterns_demo — Phase 10's UI: Factory, Visitor, and refined
# Observer (Signal<Args...>), demonstrated against Phase 2's Component
# (reused) plus this phase's own BufferGate. Links eda_patterns
# (transitively eda_oop) and eda_signal (independent).

add_executable(eda_patterns_demo
    main.cpp
)

target_link_libraries(eda_patterns_demo PRIVATE eda_patterns eda_signal)

if(MSVC)
    target_compile_options(eda_patterns_demo PRIVATE /W4)
else()
    target_compile_options(eda_patterns_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
