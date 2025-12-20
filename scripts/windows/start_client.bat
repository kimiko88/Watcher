@echo off
REM Start Classroom Control Client in background
cd /d "C:\Program Files\ClassroomControl"
start /B cms_client.exe --config "config.json"
