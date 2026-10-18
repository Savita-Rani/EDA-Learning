cmake_minimum_required(VERSION 3.15)
project(EDA_Tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(eda_sim
    src/main.cpp
    src/Gate.cpp
)

target_include_directories(eda_sim PRIVATE include)

if(MSVC)
    target_compile_options(eda_sim PRIVATE /W4)
else()
    target_compile_options(eda_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()
