# frontend/console — the current UI: a plain console driver.
# Links against eda_core and nothing else; knows nothing about Gate's
# internals, only the public <eda/...> headers.

add_executable(eda_console
    main.cpp
)

target_link_libraries(eda_console PRIVATE eda_core)

if(MSVC)
    target_compile_options(eda_console PRIVATE /W4)
else()
    target_compile_options(eda_console PRIVATE -Wall -Wextra -Wpedantic)
endif()
