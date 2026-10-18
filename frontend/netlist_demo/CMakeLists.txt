# frontend/netlist_demo — Phase 3's UI: builds a small netlist (4-NAND
# XOR) and simulates it, instead of evaluating one gate directly. Links
# eda_netlist (which pulls in eda_oop transitively via PUBLIC link).

add_executable(eda_netlist_demo
    main.cpp
)

target_link_libraries(eda_netlist_demo PRIVATE eda_netlist)

if(MSVC)
    target_compile_options(eda_netlist_demo PRIVATE /W4)
else()
    target_compile_options(eda_netlist_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
