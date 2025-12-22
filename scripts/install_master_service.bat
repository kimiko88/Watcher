@echo off
REM Installation script for Watcher Master Service
REM Must be run as Administrator

echo ====================================
echo Watcher Master Service Installation
echo ====================================
echo.

REM Check for admin privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator!
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

REM Get current directory
set "INSTALL_DIR=%~dp0"
set "SERVICE_NAME=WatcherMasterService"
set "SERVICE_EXE=%INSTALL_DIR%cms_master_service.exe"
set "GUI_EXE=%INSTALL_DIR%cms_master.exe"

echo Installation directory: %INSTALL_DIR%
echo.

REM Check if service executable exists
if not exist "%SERVICE_EXE%" (
    echo ERROR: Service executable not found: %SERVICE_EXE%
    echo Please build the project first
    pause
    exit /b 1
)

REM Check if GUI executable exists
if not exist "%GUI_EXE%" (
    echo ERROR: GUI executable not found: %GUI_EXE%
    echo Please build the project first
    pause
    exit /b 1
)

REM Stop service if already running
echo Stopping existing service (if running)...
sc stop "%SERVICE_NAME%" >nul 2>&1

REM Delete existing service
echo Removing existing service (if exists)...
sc delete "%SERVICE_NAME%" >nul 2>&1

REM Create Windows service
echo Creating Windows service...
sc create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto DisplayName= "Watcher Classroom Master Service"

if %errorLevel% neq 0 (
    echo ERROR: Failed to create service!
    pause
    exit /b 1
)

REM Set service description
sc description "%SERVICE_NAME%" "Watcher classroom monitoring master service - manages server and optionally spawns GUI"

REM Configure service recovery options
sc failure "%SERVICE_NAME%" reset= 86400 actions= restart/5000/restart/10000/restart/30000

REM Start the service
echo Starting service...
sc start "%SERVICE_NAME%"

if %errorLevel% neq 0 (
    echo WARNING: Service created but failed to start
    echo Check logs at C:\Users\Public\cms_master_service_log.txt
) else (
    echo.
    echo ====================================
    echo Installation completed successfully!
    echo ====================================
    echo.
    echo Service name: %SERVICE_NAME%
    echo Service will start automatically on boot
    echo.
    echo The GUI can be started manually by running:
    echo   %GUI_EXE% --ipc
    echo.
    echo Or the GUI will be managed by the service if auto_start_gui is enabled
    echo.
    echo Check service status with: sc query %SERVICE_NAME%
    echo View logs at: C:\Users\Public\cms_master_service_log.txt
)

echo.
pause
