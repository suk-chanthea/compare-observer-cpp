@echo off
REM Compare Observer - Build Script (FIXED)
setlocal enabledelayedexpansion

echo.
echo ========================================
echo  COMPARE OBSERVER - BUILD
echo ========================================
echo.

REM Step 1: Find Qt installation
set "QtPath="
if exist "C:\Qt\6.10.1\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.1\msvc2022_64"
    echo Found Qt 6.10.1 MSVC
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.0\msvc2022_64"
    echo Found Qt 6.10.0 MSVC
) else if exist "C:\Qt\6.9.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.9.0\msvc2022_64"
    echo Found Qt 6.9.0 MSVC
) else (
    echo ERROR: Qt with MSVC 2022 64-bit not found!
    echo Please install Qt with MSVC 2022 64-bit support.
    pause
    exit /b 1
)

echo Qt Path: %QtPath%
echo.

REM Step 2: Find Visual Studio (2026 or 2022)
set "VSDIR="
set "VS_GENERATOR="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
    set "VS_GENERATOR=Visual Studio 18 2026"
    echo Found Visual Studio 2026 Community
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Community"
    set "VS_GENERATOR=Visual Studio 17 2022"
    echo Found Visual Studio 2022 Community
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    set "VS_GENERATOR=Visual Studio 17 2022"
    echo Found Visual Studio 2022 Professional
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    set "VS_GENERATOR=Visual Studio 17 2022"
    echo Found Visual Studio 2022 Enterprise
) else (
    echo ERROR: Visual Studio not found!
    echo Please install Visual Studio 2022 or 2026 with C++ Desktop Development workload.
    pause
    exit /b 1
)

echo Visual Studio: %VSDIR%
echo.

REM Step 3: Initialize Visual Studio environment
echo Initializing Visual Studio environment...
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64

if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize Visual Studio environment
    pause
    exit /b 1
)

echo Visual Studio environment initialized successfully
echo.

REM Step 4: Clean old build
if exist build (
    echo Cleaning old build directory...
    rmdir /s /q build
)

REM Step 5: Create build directory
mkdir build
cd build

REM Step 6: Configure with CMake (using Visual Studio generator)
echo.
echo ========================================
echo  CONFIGURING CMAKE...
echo ========================================
cmake -G "%VS_GENERATOR%" -A x64 -DCMAKE_PREFIX_PATH="%QtPath%" ..

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    cd ..
    pause
    exit /b 1
)

REM Step 7: Build the project
echo.
echo ========================================
echo  BUILDING PROJECT...
echo ========================================
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed!
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo  BUILD SUCCESSFUL!
echo ========================================
echo.

REM Step 8: Find the executable
set "EXE_PATH="
if exist "bin\Release\CompareObserver.exe" (
    set "EXE_PATH=bin\Release\CompareObserver.exe"
) else if exist "bin\CompareObserver.exe" (
    set "EXE_PATH=bin\CompareObserver.exe"
) else if exist "Release\CompareObserver.exe" (
    set "EXE_PATH=Release\CompareObserver.exe"
) else (
    echo ERROR: Executable not found!
    echo Searching for it...
    dir /s /b CompareObserver.exe
    cd ..
    pause
    exit /b 1
)

echo Found executable: %EXE_PATH%
echo.

REM Step 9: Deploy Qt DLLs
echo ========================================
echo  DEPLOYING Qt DLLS...
echo ========================================
"%QtPath%\bin\windeployqt.exe" --release "%EXE_PATH%"

if %errorlevel% neq 0 (
    echo WARNING: windeployqt failed, but continuing...
)

REM Step 10: Check for OpenSSL DLLs
echo.
echo Checking for OpenSSL DLLs...
set "OPENSSL_PATH=C:\Program Files\OpenSSL-Win64\bin"
if exist "%OPENSSL_PATH%\libssl-3-x64.dll" (
    echo Copying OpenSSL DLLs...
    for %%D in (bin bin\Release Release) do (
        if exist "%%D" (
            copy "%OPENSSL_PATH%\libssl-3-x64.dll" "%%D\" 2>nul
            copy "%OPENSSL_PATH%\libcrypto-3-x64.dll" "%%D\" 2>nul
        )
    )
)

REM Step 11: Launch application
echo.
echo ========================================
echo  LAUNCHING APPLICATION...
echo ========================================
echo.

cd ..
start "" "build\%EXE_PATH%"

echo Application started successfully!
echo.
echo If you see this message but no window appears:
echo 1. Check Windows Task Manager for CompareObserver.exe
echo 2. Run manually: build\%EXE_PATH%
echo 3. Check Windows Event Viewer for errors
echo.
pause