@echo off
REM Compare Observer - Build and Run
REM This script builds the C++ Qt application

setlocal enabledelayedexpansion

echo.
echo ========================================
echo  COMPARE OBSERVER - BUILD & RUN
echo ========================================
echo.

REM Check for Qt installation
set "QtPath="
if exist "C:\Qt\6.10.1\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.1\msvc2022_64"
    echo Found Qt 6.10.1 MSVC
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.0\msvc2022_64"
    echo Found Qt 6.10.0 MSVC
) else (
    echo.
    echo ========================================
    echo  Qt MSVC NOT FOUND
    echo ========================================
    echo.
    echo You need to install Qt with MSVC 2022 64-bit support.
    echo.
    echo Option 1: Use Qt Maintenance Tool
    echo   Run: C:\Qt\MaintenanceTool.exe
    echo   Add "MSVC 2022 64-bit" to Qt 6.10.0 or 6.10.1
    echo   Takes: 10-20 minutes
    echo.
    echo Option 2: Download Fresh Qt Installer
    echo   From: https://www.qt.io/download
    echo   Install: Qt 6.10.1 with MSVC 2022 64-bit
    echo.
    pause
    exit /b 1
)

echo Qt path: !QtPath!
echo.

REM Initialize Visual Studio environment
echo Initializing Visual Studio 2026 environment...
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize Visual Studio environment
    pause
    exit /b 1
)

REM Clean old build
echo Cleaning old build...
if exist build (
    rmdir /s /q build
)

REM Create build directory
mkdir build
cd build

REM Configure with CMake
echo.
echo Configuring CMake...
cmake -DCMAKE_PREFIX_PATH="!QtPath!" -G "Visual Studio 18 2026" -A x64 ..

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Possible causes:
    echo - Qt MSVC build not found
    echo - CMake not installed
    echo - Invalid Qt path
    echo.
    pause
    exit /b 1
)

REM Build
echo.
echo Building project (this may take 2-5 minutes)...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo  BUILD SUCCESSFUL!
echo ========================================
echo.

REM Check Release build
if exist "Release\CompareObserver.exe" (
    echo Launching Compare Observer (Release)...
    timeout /t 2 /nobreak
    start Release\CompareObserver.exe
    echo Application started!
) else (
    echo ERROR: Executable not found at Release\CompareObserver.exe
    pause
    exit /b 1
)

echo ========================================
echo  Done!
echo ========================================
echo.
pause
