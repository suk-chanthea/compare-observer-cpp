@echo off
REM Qt Maintenance Tool - Add MSVC Support
REM This script will open Qt Maintenance Tool to add Windows MSVC support

setlocal enabledelayedexpansion

echo.
echo ========================================
echo  Qt MAINTENANCE TOOL - Add MSVC Support
echo ========================================
echo.

set "QtMaintenance=C:\Qt\MaintenanceTool.exe"

if not exist "%QtMaintenance%" (
    echo ERROR: Qt Maintenance Tool not found at:
    echo %QtMaintenance%
    echo.
    echo Please download Qt installer from:
    echo https://www.qt.io/download-open-source
    pause
    exit /b 1
)

echo Opening Qt Maintenance Tool...
echo.
echo INSTRUCTIONS:
echo 1. Click "Add or remove components"
echo 2. Expand: Qt
echo 3. Select version: 6.10.0 or 6.10.1
echo 4. IMPORTANT: Check "MSVC 2022 64-bit"
echo 5. Click "Next" and wait for installation
echo 6. This will download ~500MB - 1GB
echo.
echo Press any key to continue...
pause

start "" "%QtMaintenance%"

echo.
echo Qt Maintenance Tool started!
echo After installation completes, close this window.
echo Then run: build_app.cmd
echo.
pause
