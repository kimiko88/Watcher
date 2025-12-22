@echo off
REM Uninstallation script for Watcher Client Service
REM Must be run as Administrator

echo ========================================
echo Watcher Client Service Uninstallation
echo ========================================
echo.

REM Check for admin privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator!
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

set "SERVICE_NAME=WatcherClientService"

REM Stop the service
echo Stopping service...
sc stop "%SERVICE_NAME%"

if %errorLevel% neq 0 (
    echo WARNING: Service may not be running
)

REM Wait a moment for service to stop
timeout /t 3 /nobreak >nul

REM Delete the service
echo Deleting service...
sc delete "%SERVICE_NAME%"

if %errorLevel% neq 0 (
    echo ERROR: Failed to delete service!
    echo The service may not exist or may still be running
    pause
    exit /b 1
)

echo.
echo ========================================
echo Uninstallation completed successfully!
echo ========================================
echo.
echo The service has been removed from Windows
echo.

pause
