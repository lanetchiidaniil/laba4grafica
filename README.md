# OpenGL Lab 4

3D scene inspired by a rural outhouse / Shrek toilet concept rendered in OpenGL. The project uses a custom textured cube-based scene with a camera, lighting, depth testing, and procedural/generated texture fallback.

## Included technology

- GLFW
- GLAD
- GLM
- stb_image
- CMake + FetchContent
- MinGW toolchain on Windows
- custom shader pipeline
- camera movement and mouse look
- textured 3D geometry with lighting and depth testing

## Current scene

The scene is a stylized outdoor outhouse with:
- a tall, narrow body resembling a countryside toilet
- pitched roof elements
- a platform/base
- wooden-texture material treatment
- atmospheric forest-like background color and ground plane
- generated texture fallback if a texture file is missing

## Project structure

- `src/main.cpp` — main scene, camera, rendering loop, object transforms
- `src/Shader.cpp` / `src/Shader.h` — shader loading and compilation
- `src/Camera.cpp` / `src/Camera.h` — camera controls
- `src/Texture.cpp` / `src/Texture.h` — texture loading and binding
- `shaders/vertex.glsl` — vertex shader
- `shaders/fragment.glsl` — fragment shader
- `textures/` — asset folder for textures
- `rebuild_and_run.bat` — automated clean rebuild and start script

## Build and run

### Recommended Windows command

Double-click the file:

```bat
rebuild_and_run.bat
```

This script:
- removes the old `build` folder
- configures CMake with the MinGW toolchain explicitly
- builds the project
- checks that `OpenGL_Lab4.exe` was produced
- launches the app

### Manual build

From the project root:

```bat
"C:\Program Files\CodeBlocks\MinGW\bin\cmake.exe" -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:\Program Files\CodeBlocks\MinGW\bin\gcc.exe" -DCMAKE_CXX_COMPILER="C:\Program Files\CodeBlocks\MinGW\bin\g++.exe" -DCMAKE_MAKE_PROGRAM="C:\Program Files\CodeBlocks\MinGW\bin\mingw32-make.exe"
"C:\Program Files\CodeBlocks\MinGW\bin\cmake.exe" --build build -- -j4
```

Then run:

```bat
build\OpenGL_Lab4.exe
```

## Controls

- W / S / A / D — camera movement
- Q / E — vertical camera movement
- Arrow keys — camera rotation
- Left mouse button + drag — mouse look
- ESC — exit

## Notes

- Dependencies are downloaded automatically via `FetchContent`, so the project does not require manually installing them into the system.
- The build is configured for Windows with MinGW.
- The project includes a fallback generated texture so it can still render even if an external texture is missing.
- The current visual goal is a Shrek-style rural outhouse / toilet composition inspired by the provided reference.
