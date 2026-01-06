@echo off
REM Quick Build Script for Watcher Service Architecture
REM Run this after cleaning build directory

setlocal EnableDelayedExpansion

REM SET YOUR CUSTOM QT PATH HERE
set "USER_QT_PATH=C:\Qt\6.10.1\msvc2022_64"

echo ====================================
echo Watcher Service Architecture Build
echo ====================================
echo.

REM Check for admin (optional, but recommended)
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running as Administrator: YES
) else (
    echo Running as Administrator: NO
    echo Warning: Some operations may require admin privileges
)
echo.

REM Step 1: Clean build directory
if exist build (
    echo Cleaning build directory...
    rmdir /s /q build
)

mkdir build
cd build

REM Step 2: Configure with CMake
echo.
echo ====================================
echo Step 1: CMake Configuration
echo ====================================
echo.

if defined USER_QT_PATH (
    if exist "%USER_QT_PATH%" (
        echo Using user-defined Qt path: %USER_QT_PATH%
        set "QT_PATH=%USER_QT_PATH%"
        goto :qt_found
    ) else (
        echo WARNING: USER_QT_PATH defined but not found: %USER_QT_PATH%
        echo Attempting auto-detection...
    )
)

REM Try to find Qt automatically
set "QT_PATH="
for %%D in (C:\Qt\6.* C:\Qt6\6.*) do (
    if exist "%%D\msvc2019_64" (
        set "QT_PATH=%%D\msvc2022_64"
        goto :qt_found
    )
)

:qt_found
if defined QT_PATH (
    echo Found Qt at: %QT_PATH%
    cmake .. -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="%QT_PATH%"
) else (
    echo Qt not found automatically, trying without Qt path...
    echo (Master GUI will not build without Qt)
    cmake .. -G "Visual Studio 17 2022" -A x64
)

if %errorLevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Possible fixes:
    echo 1. Install Qt 6.5+ from https://www.qt.io/
    echo 2. Set Qt path manually:
    echo    cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"
    echo 3. Check Visual Studio 2022 is installed
    echo.
    pause
    exit /b 1
)

echo.
echo ====================================
echo Step 2: Building Core Library
echo ====================================
echo.

cmake --build . --target cms_core --config Release

if %errorLevel% neq 0 (
    echo ERROR: Core build failed!
    echo Check errors above
    pause
    exit /b 1
)

echo Core build successful!
echo.

REM Step 3: Build Platform
echo ====================================
echo Step 3: Building Platform Library
echo ====================================
echo.

cmake --build . --target cms_platform --config Release

if %errorLevel% neq 0 (
    echo ERROR: Platform build failed!
    pause
    exit /b 1
)

echo.

REM Step 4: Build Client Service
echo ====================================
echo Step 4: Building Client Service
echo ====================================
echo.

cmake --build . --target cms_client_service --config Release

if %errorLevel% neq 0 (
    echo WARNING: Client service build failed
    echo Continuing anyway...
) else (
    echo Client service build successful!
)

echo.

REM Step 5: Build Client Worker
echo ====================================
echo Step 5: Building Client Worker
echo ====================================
echo.

cmake --build . --target cms_client_worker --config Release

if %errorLevel% neq 0 (
    echo WARNING: Client worker build failed
) else (
    echo Client worker build successful!
)

echo.

REM Step 6: Build Master (if Qt available)
echo ====================================
echo Step 6: Building Master Components
echo ====================================
echo.

cmake --build . --target cms_master_service --config Release
if %errorLevel% == 0 (
    echo Master service build successful!
)

cmake --build . --target cms_master --config Release
if %errorLevel% == 0 (
    echo Master GUI build successful!
)

echo.
echo ====================================
echo Build Summary
echo ====================================
echo.

set BUILD_SUCCESS=0

if exist "Release\cms_client_service.exe" (
    echo [OK] cms_client_service.exe
    set /a BUILD_SUCCESS+=1
) else (
    echo [FAIL] cms_client_service.exe
)

if exist "Release\cms_client_worker.exe" (
    echo [OK] cms_client_worker.exe
    set /a BUILD_SUCCESS+=1
) else (
    echo [FAIL] cms_client_worker.exe
)

if exist "Release\cms_master_service.exe" (
    echo [OK] cms_master_service.exe
    set /a BUILD_SUCCESS+=1
) else (
    echo [FAIL] cms_master_service.exe
)

if exist "Release\cms_master.exe" (
    echo [OK] cms_master.exe
    set /a BUILD_SUCCESS+=1
) else (
    echo [FAIL] cms_master.exe (Qt required)
)

echo.
echo Built %BUILD_SUCCESS% out of 4 executables
echo.

if %BUILD_SUCCESS% GEQ 2 (
    echo ====================================
    echo Next Steps
    echo ====================================
    echo.
    echo To install services, run:
    echo   cd Release
    echo   ..\..\scripts\install_client_service.bat
    echo   ..\..\scripts\install_master_service.bat
    echo.
    echo To test IPC:
    echo   ctest -C Release --output-on-failure
    echo.
) else (
    echo Build incomplete. Check errors above.
)

pause
