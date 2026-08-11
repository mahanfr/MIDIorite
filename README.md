# Midiorite

> **Note:** This README was generated with the assistance of AI.

**Midiorite** is a digital music synthesizer written in C using [raylib](https://www.raylib.com/), combining real-time audio synthesis with custom graphics and visual effects.

The project focuses on creating a synthesizer that is not only functional but also visually interesting, with graphics that react to the generated music.

## Features

- Real-time digital audio synthesis
- Interactive synthesizer interface
- Real-time audio visualization
- Custom graphics and visual effects
- Cross-platform builds
- Embedded assets for single-executable distribution
- Written in C
- Powered by raylib

## Demo

A demonstration video will be added here.

https://github.com/user-attachments/assets/2aa48644-b782-45e5-86b6-072c989725e4

## Building

Midiorite uses **CMake** as its build system.

The project includes prebuilt/platform-specific raylib libraries under:

```text
libs/raylib/
├── linux/
├── win32-mingw/
└── win32-msvc/
```

## Requirements

You need:

* CMake 3.20 or newer
* A C99-compatible C compiler
* raylib libraries included in the repository

For Linux:

* GCC
* OpenGL
* X11 development libraries
* pthread
* dl
* rt

For Windows/MinGW:

* MinGW-w64

For Windows/MSVC:

* Visual Studio with the C/C++ development tools
* CMake

## Build

In order to build this project for your platform please follow the instructions provided below.

### Linux

Configure the project:

``` bash
cmake -S . -B build
```

Build:

``` bash
cmake --build build
```

The executable will be located at:

```
build/midiorite
```

Run it with:

``` bash
./build/midiorite
```

### Windows — MinGW

MinGW builds can also be performed from Linux using the MinGW cross compiler.

First make sure x86_64-w64-mingw32-gcc is installed.

Configure:

``` powershell
cmake -S . -B build-mingw -DMINGW=ON
```

Build:

``` powershell
cmake --build build-mingw
```

The resulting executable will be:

```
build-mingw/midiorite.exe
```

The same -DMINGW=ON configuration can be used when building on a Windows system with MinGW installed.

### Windows — MSVC

Open a Visual Studio Developer Command Prompt and configure the project:

``` powershell
cmake -S . -B build-msvc
```

Build:

``` powershell
cmake --build build-msvc --config Release
```

The executable will be located at:

```
build-msvc\Release\midiorite.exe
```

For a 64-bit Visual Studio generator, you can explicitly select the architecture:

``` powershell
cmake -S . -B build-msvc -A x64
cmake --build build-msvc --config Release
```

# License

See the repository's license file for licensing information.
