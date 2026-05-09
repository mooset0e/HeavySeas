# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Heavy Seas is a Sid Meier's Pirates! clone built from scratch in C++ using SDL2.

## Build

```powershell
# Configure (only needed once or when CMakeLists.txt changes)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\Users\awmum\vcpkg\scripts\buildsystems\vcpkg.cmake

# Build
cmake --build build

# Run
.\build\Debug\HeavySeas.exe
```

SDL2 is managed via vcpkg at `C:\Users\awmum\vcpkg`. To add a new library, run `.\vcpkg install <pkg>:x64-windows` from that directory and add it to `vcpkg.json` and `CMakeLists.txt`.

## Architecture

All source lives under `src/`. The entry point is `src/main.cpp`, which owns the SDL2 window, renderer, and main game loop.

The intended architecture as the game grows:

- `src/core/` — game loop, timing, input handling
- `src/world/` — world map, ports, sea regions
- `src/ship/` — ship stats, sailing mechanics
- `src/combat/` — ship combat system
- `src/trade/` — economy, cargo, trading
- `src/crew/` — crew morale, recruitment
- `src/ui/` — HUD, menus, dialogue

## Tech

- **Language:** C++17
- **Renderer:** SDL2 (software + accelerated via `SDL_Renderer`)
- **Build:** CMake + MSVC (Visual Studio 2026 Build Tools)
- **Dependencies:** vcpkg (`vcpkg.json` locks versions)
- **Platform:** Windows x64
