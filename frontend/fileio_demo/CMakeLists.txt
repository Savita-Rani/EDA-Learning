# frontend/fileio_demo — Phase 5's UI: loads the same 4-NAND XOR circuit
# as Phases 3/4's demos, but from circuits/xor4nand.nl instead of
# addComponent() calls. Links eda_fileio (transitively eda_netlist, eda_oop).

add_executable(eda_fileio_demo
    main.cpp
)

target_link_libraries(eda_fileio_demo PRIVATE eda_fileio)

# Absolute path to this directory's circuits/ folder, baked in at compile
# time — works regardless of the executable's current working directory,
# no file-copying into build/bin needed.
target_compile_definitions(eda_fileio_demo PRIVATE
    CIRCUIT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/circuits"
)

if(MSVC)
    target_compile_options(eda_fileio_demo PRIVATE /W4)
else()
    target_compile_options(eda_fileio_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
