cmake_minimum_required(VERSION 3.10)
project(SecondBurst C)

set(CMAKE_C_STANDARD 99)

set(SOURCES
    src/main.c
    src/window.c
    src/ui.c
    src/app.c
)

set(HEADERS
    include/window.h
    include/ui.h
    include/app.h
)

add_executable(SecondBurst WIN32 ${SOURCES} ${HEADERS})

target_include_directories(SecondBurst PRIVATE include)

target_link_libraries(SecondBurst gdi32 user32 kernel32)

set_target_properties(SecondBurst PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    set_target_properties(SecondBurst PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup"
    )
endif()