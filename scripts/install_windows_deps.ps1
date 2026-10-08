# Run as Administrator if needed
$ErrorActionPreference = "Stop"

Write-Host "Installing CMake and C++ build tools for VS Code..."

winget install --id Kitware.CMake --exact --accept-source-agreements --accept-package-agreements
winget install --id MSYS2.MSYS2 --exact --accept-source-agreements --accept-package-agreements

Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Restart VS Code"
Write-Host "2. Open this workspace"
Write-Host "3. Install C/C++ and CMake Tools extensions if needed"
Write-Host "4. Run: cmake -S . -B build"
Write-Host "5. Run: cmake --build build"
