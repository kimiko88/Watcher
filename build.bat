@echo off
REM Build script for Classroom Control (CMS)
REM This script sets up Visual Studio environment and builds the project

echo ============================================
echo Classroom Control (CMS) - Build Script
echo ============================================
echo.

REM Find and run VsDevCmd.bat
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Could not find Visual Studio Developer Command Prompt
    echo Please ensure Visual Studio 2022 is installed
    pause
    exit /b 1
)

echo Visual Studio environment configured
echo.

REM Build the project
echo Building project...
cmake --build build --config Debug
if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build completed successfully!
echo ============================================
echo.
echo To run tests:
echo   cd build
echo   ctest --output-on-failure -C Debug
echo.
echo Or run specific test suites:
echo   .\tests\Debug\cms_unit_tests.exe
echo   .\tests\Debug\cms_integration_tests.exe
echo.
pause
