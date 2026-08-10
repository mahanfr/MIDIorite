cmake_minimum_required(VERSION 3.10)
project(midi-orite C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(COPY ${CMAKE_SOURCE_DIR}/assets
    DESTINATION ${CMAKE_BINARY_DIR}/bin
)

if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES main.c)

add_executable(midi-orite ${SOURCES} ${ASSETS})

target_link_libraries(midi-orite raylib m)

if(UNIX AND NOT WIN32)
    target_link_libraries(midi-orite
        GL pthread dl rt X11)
elseif(WIN32)
    if(MINGW)
        target_link_libraries(midi-orite
            winmm gdi32 opengl32)
    elseif(MSVC)
        target_link_libraries(midi-orite
            winmm.lib gdi32.lib opengl32.lib)
    endif()
endif()

set_target_properties(midi-orite PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(UNIX AND NOT WIN32)
    set_target_properties(midi-orite PROPERTIES
        INSTALL_RPATH "${CMAKE_SOURCE_DIR}/libs"
    )
endif()
