# frontend/generic_demo — Phase 6's UI: prints truth tables for the same
# AND/OR/NOT/NAND/NOR/XOR gates, via ONE template (LogicGate<T>)
# instantiated for both `bool` and `LogicValue`. Links eda_generic.

add_executable(eda_generic_demo
    main.cpp
)

target_link_libraries(eda_generic_demo PRIVATE eda_generic)

if(MSVC)
    target_compile_options(eda_generic_demo PRIVATE /W4)
else()
    target_compile_options(eda_generic_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
