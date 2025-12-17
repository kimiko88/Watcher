@echo off
REM Quick build script with fix for MSVC /RTC1 conflict
REM Run this from Developer Command Prompt for VS 2022

echo ========================================
echo CMS Project - Quick Build Script
echo ========================================
echo.

REM Configure
echo Configuring CMake...
cmake -B build -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Configuration successful!
echo.

REM Build
echo Building project...
cmake --build build --config Debug --parallel 4
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo To run tests:
echo   cd build
echo   ctest --output-on-failure -C Debug
echo.
pause
