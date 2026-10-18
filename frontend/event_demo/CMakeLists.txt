# frontend/event_demo — Phase 4's UI: the same 4-NAND XOR circuit as
# Phase 3's netlist_demo, driven by EventSimulator instead of Netlist.
# Links eda_event (which pulls in eda_oop transitively).

add_executable(eda_event_demo
    main.cpp
)

target_link_libraries(eda_event_demo PRIVATE eda_event)

if(MSVC)
    target_compile_options(eda_event_demo PRIVATE /W4)
else()
    target_compile_options(eda_event_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
