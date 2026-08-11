cmake_minimum_required(VERSION 3.20)
option(MINGW "Build for Windows using MinGW" OFF)

if(MINGW)
    set(CMAKE_SYSTEM_NAME Windows)

    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
endif()

project(midiorite VERSION 1.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
else()
    add_compile_options(/W4 /WX-)
endif()

add_executable(midiorite main.c)

target_include_directories(midiorite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)

    if(MSVC)

        message(STATUS "Building Windows / MSVC")

        set(RAYLIB_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/libs/raylib/win32-msvc
        )

        target_link_libraries(midiorite PRIVATE
            ${RAYLIB_DIR}/raylib.lib
            winmm
            gdi32
            opengl32
        )

        set_target_properties(midiorite PROPERTIES
            WIN32_EXECUTABLE TRUE
        )

        target_link_options(midiorite PRIVATE
            /SUBSYSTEM:WINDOWS 
            /ENTRY:mainCRTStartup
        )

    elseif(MINGW)

        message(STATUS "Building Windows / MinGW")

        set(RAYLIB_DIR
            ${CMAKE_CURRENT_SOURCE_DIR}/libs/raylib/win32-mingw
        )

        target_link_libraries(midiorite PRIVATE
            ${RAYLIB_DIR}/libraylib.a
            winmm
            gdi32
            opengl32
            mingwex
        )

        target_link_options(midiorite PRIVATE
            -Wl,--defsym,stat64i32=_stat64
            -Wl,--subsystem,windows
        )

    endif()

elseif(UNIX)

    message(STATUS "Building Linux")

    set(RAYLIB_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/raylib/linux
    )

    target_link_libraries(midiorite PRIVATE
        ${RAYLIB_DIR}/libraylib.a
        m
        GL
        pthread
        dl
        rt
        X11
    )

endif()

# Embed assets

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedAssets.cmake)

embed_assets(
    midiorite
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
)
