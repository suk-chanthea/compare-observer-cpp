@echo off
REM Compare Observer - Build and Run (FIXED)
REM This script builds the C++ Qt application AND deploys Qt DLLs

setlocal enabledelayedexpansion

echo.
echo ========================================
echo  COMPARE OBSERVER - BUILD & RUN (FIXED)
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

REM Check if windeployqt exists
set "WINDEPLOYQT=!QtPath!\bin\windeployqt.exe"
if not exist "!WINDEPLOYQT!" (
    echo ERROR: windeployqt.exe not found at !WINDEPLOYQT!
    pause
    exit /b 1
)

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

REM Check for executable in bin directory (as per CMakeLists.txt)
set "EXE_PATH="
if exist "bin\CompareObserver.exe" (
    set "EXE_PATH=bin\CompareObserver.exe"
    echo Found executable at: bin\CompareObserver.exe
) else if exist "Release\CompareObserver.exe" (
    set "EXE_PATH=Release\CompareObserver.exe"
    echo Found executable at: Release\CompareObserver.exe
) else (
    echo ERROR: Executable not found!
    echo Searched in:
    echo   - bin\CompareObserver.exe
    echo   - Release\CompareObserver.exe
    dir /s /b CompareObserver.exe
    pause
    exit /b 1
)

REM Deploy Qt DLLs using windeployqt (CRITICAL FIX!)
echo.
echo ========================================
echo  DEPLOYING Qt DLLs...
echo ========================================
echo.
echo Running windeployqt to copy required Qt DLLs...
"!WINDEPLOYQT!" --release "!EXE_PATH!"

if %errorlevel% neq 0 (
    echo.
    echo WARNING: windeployqt failed, but continuing...
    echo You may need to manually copy Qt DLLs
)

REM Also copy OpenSSL DLLs if needed
echo.
echo Checking for OpenSSL DLLs...
set "OPENSSL_PATH=C:\Program Files\OpenSSL-Win64\bin"
if exist "!OPENSSL_PATH!\libssl-3-x64.dll" (
    echo Copying OpenSSL DLLs...
    copy "!OPENSSL_PATH!\libssl-3-x64.dll" "bin\" 2>nul
    copy "!OPENSSL_PATH!\libcrypto-3-x64.dll" "bin\" 2>nul
) else (
    echo OpenSSL DLLs not found at !OPENSSL_PATH!
    echo If the app fails to start, install OpenSSL from:
    echo https://slproweb.com/products/Win32OpenSSL.html
)

echo.
echo ========================================
echo  LAUNCHING APPLICATION...
echo ========================================
echo.
timeout /t 2 /nobreak

REM Launch the executable
start "" "!EXE_PATH!"

echo.
echo ========================================
echo  Application started!
echo ========================================
echo.
echo If the app doesn't show a window:
echo 1. Check Windows Event Viewer for errors
echo 2. Run from command line to see error messages:
echo    cd build
echo    !EXE_PATH!
echo.
pause