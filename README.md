# OpenGL Lab 4

This project is a stylized OpenGL scene inspired by a Shrek-like character in a fenced yard with an outhouse, grass, rocks, and interactive objects. The scene includes procedural geometry, character animation, camera orbit controls, and simple gameplay mechanics.

## Features

- Shrek-inspired character with head, body, boots, face, and arm animation
- Third-person camera orbit around the character
- WASD movement for character navigation
- Q/E rotation in place for facing adjustments
- Interactive door opening near the outhouse
- Press `G` to run the complete outhouse sequence: Shrek walks to the door, enters, closes it, waits two seconds, then exits facing the camera and closes the door
- Stone pickup interaction near a rock
- Throw mechanic using T while holding the stone
- Arm animation with R to raise the left arm from the shoulder, with a visible elbow
- Procedural environment with grass, fence, rocks, and decorative objects

## Tech stack

- C++17
- OpenGL
- GLFW
- GLAD
- GLM
- CMake
- MinGW on Windows

## Project structure

- `src/main.cpp` — main rendering loop, scene logic, character animation, interaction system
- `src/Camera.h` / `src/Camera.cpp` — camera orbit and view matrix logic
- `src/Shader.h` / `src/Shader.cpp` — shaders loading and setup
- `src/Texture.h` / `src/Texture.cpp` — texture loading and binding
- `shaders/vertex.glsl` — vertex shader
- `shaders/fragment.glsl` — fragment shader
- `textures/` — scene textures and assets
- `assets/` — music and sound effects used by the scene
- `rebuild_and_run.bat` — automated rebuild and launch helper
- `generate_shrek_outhouse.py` — procedural scene generator helper

## Build and run

### Windows (recommended)

Run the helper script from the project root:

```bat
rebuild_and_run.bat
```

This script rebuilds the project with the MinGW toolchain and launches the app.

### Manual build

```bat
"C:\Program Files\CodeBlocks\MinGW\bin\cmake.exe" -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:\Program Files\CodeBlocks\MinGW\bin\gcc.exe" -DCMAKE_CXX_COMPILER="C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="C:\Program Files\CodeBlocks\MinGW\bin\mingw32-make.exe"
"C:\Program Files\CodeBlocks\MinGW\bin\cmake.exe" --build build -- -j4
```

Then run:

```bat
build\OpenGL_Lab4.exe
```

## Controls

- `W / S / A / D` — character movement
- `Q / E` — turn in place
- `G` — run the full outhouse entrance and exit sequence from anywhere in the yard
- `F` — open or close the nearby door, or pick up a nearby stone
- `T` — throw the held stone
- `R` — raise or lower the left arm from the shoulder
- Exit music starts from the beginning during the two-second wait inside, so the vocal begins around the door opening
- Audio cues: `shrek_sigh.mp3` on approach, `belly_growl.mp3` during the stomach rub, `stone_bounce.mp3` on the stone's first impact (limited to two seconds), and `shrek_victory.mp3` for the final gesture
- Mouse — camera orbit / view adjustment
- `ESC` — exit

## Notes

- The project is configured for Windows with MinGW.
- Audio files are included in `assets/`: `All Star.mp3`, `shrek_sigh.mp3`, `belly_growl.mp3`, `stone_bounce.mp3`, and `shrek_victory.mp3`.
- The build uses CMake and downloads dependencies automatically via `FetchContent`.
- The current project version contains a custom character and environment built from basic geometry, not from a prebuilt imported 3D model.
- The scene is intended as a small interactive graphical demo rather than a full game engine.
