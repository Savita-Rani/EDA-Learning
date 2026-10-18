# frontend/memory_demo — Phase 7's UI: demonstrates OwningPtr<T> and
# SharedOwningPtr<T> owning Phase 2's Component, and FileHandle's RAII
# surviving an exception. Links both eda_memory (the generic smart
# pointers/RAII types) and eda_oop (Component, as something to own) —
# linking both here does NOT make eda_memory itself depend on eda_oop.

add_executable(eda_memory_demo
    main.cpp
)

target_link_libraries(eda_memory_demo PRIVATE eda_memory eda_oop)

if(MSVC)
    target_compile_options(eda_memory_demo PRIVATE /W4)
else()
    target_compile_options(eda_memory_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
