@echo off
REM ============================================================
REM Quick Launch Script for cms_master.exe
REM ============================================================
REM Automatically deploys Qt6 DLLs if missing, then runs cms_master

echo.
echo Starting cms_master...
echo.

REM Check if Qt6 DLLs are already deployed
if not exist "build\src\master\Release\Qt6Core.dll" (
    echo Qt6 runtime libraries not found. Running deployment...
    echo.
    call deploy_master.bat
    if errorlevel 1 (
        echo.
        echo Deployment failed! Cannot start cms_master.
        pause
        exit /b 1
    )
)

REM Run the master application
echo.
echo Launching cms_master.exe...
echo.
cd build\src\master\Release
start "" cms_master.exe

echo.
echo cms_master launched!
echo.
