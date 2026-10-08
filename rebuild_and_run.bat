@echo off
setlocal
cd /d "%~dp0"

set "MINGW=C:\Program Files\CodeBlocks\MinGW\bin"
set "PATH=%MINGW%;%PATH%"
set "CC=%MINGW%\gcc.exe"
set "CXX=%MINGW%\g++.exe"
set "CMAKE_MAKE_PROGRAM=%MINGW%\mingw32-make.exe"

rmdir /s /q build 2>nul

"%MINGW%\cmake.exe" -S . -B build -G "MinGW Makefiles" ^
  -DCMAKE_C_COMPILER="%CC%" ^
  -DCMAKE_CXX_COMPILER="%CXX%" ^
  -DCMAKE_MAKE_PROGRAM="%CMAKE_MAKE_PROGRAM%"
if errorlevel 1 (
    echo CONFIG_ERROR
    exit /b %errorlevel%
)

"%MINGW%\cmake.exe" --build build -- -j4
if errorlevel 1 (
    echo BUILD_ERROR
    exit /b %errorlevel%
)

if not exist ".\build\OpenGL_Lab4.exe" (
    echo EXE_NOT_CREATED
    exit /b 1
)

echo BUILD_OK
start "" ".\build\OpenGL_Lab4.exe"
