# frontend/parallel_demo — Phase 8's UI: correctness check (4-NAND XOR)
# plus an honestly-labeled artificial speedup demonstration. Links
# eda_parallel (transitively eda_oop + eda_threadpool).

add_executable(eda_parallel_demo
    main.cpp
)

target_link_libraries(eda_parallel_demo PRIVATE eda_parallel)

if(MSVC)
    target_compile_options(eda_parallel_demo PRIVATE /W4)
else()
    target_compile_options(eda_parallel_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
