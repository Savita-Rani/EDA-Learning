# frontend/imgui_demo — Phase 9's UI.
#
# Dear ImGui ships as SOURCE, meant to be compiled directly into your
# own project (there's no "ImGui.dll" to link) — fetched here via
# FetchContent, pinned to a specific release tag for reproducibility.
# GLFW (windowing/input) comes from MSYS2's pacman
# (mingw-w64-ucrt-x86_64-glfw); OpenGL comes from the system
# (opengl32, always present on Windows via the MinGW toolchain).
#
# Depends on eda_netlist (Phase 3's Netlist, reused) and this phase's
# own eda_waveform.

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.93.0
)
FetchContent_MakeAvailable(imgui)

# Third-party code — deliberately NOT given this project's -Wall/-Wextra/
# -Wpedantic flags; those are for code we own, not vendored sources.
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(imgui PUBLIC glfw opengl32)

add_executable(eda_imgui_demo
    main.cpp
)

target_link_libraries(eda_imgui_demo PRIVATE eda_netlist eda_waveform imgui)

if(MSVC)
    target_compile_options(eda_imgui_demo PRIVATE /W4)
else()
    target_compile_options(eda_imgui_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
