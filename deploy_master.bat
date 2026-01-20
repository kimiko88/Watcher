@echo off
REM ============================================================
REM Deploy Script for cms_master.exe - Qt6 Runtime Dependencies
REM ============================================================
REM This script copies all necessary Qt6 DLLs to the Release directory
REM so that cms_master.exe can run without Qt6 in the system PATH.

setlocal enabledelayedexpansion

echo.
echo ========================================
echo  cms_master Deployment Script
echo ========================================
echo.

REM Configuration
set QT_PATH=C:\Qt\6.10.1\msvc2022_64
set EXE_DIR=build\src\master\Release
set DEPLOY_DIR=%EXE_DIR%

REM Check if Qt6 installation exists
if not exist "%QT_PATH%\bin\Qt6Core.dll" (
    echo ERROR: Qt6 installation not found at: %QT_PATH%
    echo.
    echo Please update QT_PATH in this script to match your Qt installation.
    echo Common paths:
    echo   - C:\Qt\6.10.1\msvc2022_64
    echo   - C:\Qt\6.6.0\msvc2019_64
    echo.
    pause
    exit /b 1
)

REM Check if executable exists
if not exist "%EXE_DIR%\cms_master.exe" (
    echo ERROR: cms_master.exe not found at: %EXE_DIR%
    echo.
    echo Please build the project first using build_all.bat
    echo.
    pause
    exit /b 1
)

echo Qt6 Path: %QT_PATH%
echo Deploy Directory: %DEPLOY_DIR%
echo.

REM Required Qt6 Core DLLs
echo Copying Qt6 Core DLLs...
copy /Y "%QT_PATH%\bin\Qt6Core.dll" "%DEPLOY_DIR%\" >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] Qt6Core.dll
) else (
    echo   [OK] Qt6Core.dll
)

copy /Y "%QT_PATH%\bin\Qt6Gui.dll" "%DEPLOY_DIR%\" >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] Qt6Gui.dll
) else (
    echo   [OK] Qt6Gui.dll
)

copy /Y "%QT_PATH%\bin\Qt6Widgets.dll" "%DEPLOY_DIR%\" >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] Qt6Widgets.dll
) else (
    echo   [OK] Qt6Widgets.dll
)

copy /Y "%QT_PATH%\bin\Qt6Network.dll" "%DEPLOY_DIR%\" >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] Qt6Network.dll
) else (
    echo   [OK] Qt6Network.dll
)

echo.

REM Platform plugin (REQUIRED for Qt GUI applications)
echo Copying Qt6 platform plugins...
if not exist "%DEPLOY_DIR%\platforms" mkdir "%DEPLOY_DIR%\platforms"
copy /Y "%QT_PATH%\plugins\platforms\qwindows.dll" "%DEPLOY_DIR%\platforms\" >nul 2>&1
if errorlevel 1 (
    echo   [FAIL] platforms\qwindows.dll
) else (
    echo   [OK] platforms\qwindows.dll
)

echo.

REM Style plugins (optional but recommended)
echo Copying Qt6 style plugins...
if not exist "%DEPLOY_DIR%\styles" mkdir "%DEPLOY_DIR%\styles"
copy /Y "%QT_PATH%\plugins\styles\qwindowsvistastyle.dll" "%DEPLOY_DIR%\styles\" >nul 2>&1
if errorlevel 1 (
    echo   [WARN] styles\qwindowsvistastyle.dll (optional)
) else (
    echo   [OK] styles\qwindowsvistastyle.dll
)

echo.

REM Visual C++ Runtime (often needed)
echo Copying Visual C++ Runtime DLLs...
set VCREDIST_DLLS=msvcp140.dll vcruntime140.dll vcruntime140_1.dll

for %%D in (%VCREDIST_DLLS%) do (
    if exist "%QT_PATH%\bin\%%D" (
        copy /Y "%QT_PATH%\bin\%%D" "%DEPLOY_DIR%\" >nul 2>&1
        echo   [OK] %%D
    ) else (
        REM Try system32
        if exist "C:\Windows\System32\%%D" (
            copy /Y "C:\Windows\System32\%%D" "%DEPLOY_DIR%\" >nul 2>&1
            echo   [OK] %%D ^(from System32^)
        ) else (
            echo   [SKIP] %%D ^(not found^)
        )
    )
)

echo.
echo ========================================
echo  Deployment Complete!
echo ========================================
echo.
echo You can now run cms_master.exe from:
echo   %DEPLOY_DIR%\cms_master.exe
echo.
echo Or use the run_master.bat shortcut script.
echo.

pause
