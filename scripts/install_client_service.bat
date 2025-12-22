@echo off
REM Installation script for Watcher Client Service
REM Must be run as Administrator

echo ====================================
echo Watcher Client Service Installation
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
set "SERVICE_NAME=WatcherClientService"
set "SERVICE_EXE=%INSTALL_DIR%cms_client_service.exe"
set "WORKER_EXE=%INSTALL_DIR%cms_client_worker.exe"

echo Installation directory: %INSTALL_DIR%
echo.

REM Check if service executable exists
if not exist "%SERVICE_EXE%" (
    echo ERROR: Service executable not found: %SERVICE_EXE%
    echo Please build the project first
    pause
    exit /b 1
)

REM Check if worker executable exists
if not exist "%WORKER_EXE%" (
    echo ERROR: Worker executable not found: %WORKER_EXE%
    echo Please build the project first
    pause
    exit /b 1
)

REM Stop service if already running
echo Stopping existing service (if running)...
sc stop "%SERVICE_NAME%" >nul 2>&1

REM Delete existing service
echo Removing existing  service (if exists)...
sc delete "%SERVICE_NAME%" >nul 2>&1

REM Create Windows service
echo Creating Windows service...
sc create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto DisplayName= "Watcher Classroom Client Service"

if %errorLevel% neq 0 (
    echo ERROR: Failed to create service!
    pause
    exit /b 1
)

REM Set service description
sc description "%SERVICE_NAME%" "Watcher classroom monitoring client service - manages client worker process with IPC communication"

REM Configure service recovery options (restart on failure)
sc failure "%SERVICE_NAME%" reset= 86400 actions= restart/5000/restart/10000/restart/30000

REM Start the service
echo Starting service...
sc start "%SERVICE_NAME%"

if %errorLevel% neq 0 (
    echo WARNING: Service created but failed to start
    echo Check logs at C:\Users\Public\cms_service_log.txt
) else (
    echo.
    echo ====================================
    echo Installation completed successfully!
    echo ====================================
    echo.
    echo Service name: %SERVICE_NAME%
    echo Service will start automatically on boot
    echo.
    echo Check service status with: sc query %SERVICE_NAME%
    echo View logs at: C:\Users\Public\cms_service_log.txt
)

echo.
pause
