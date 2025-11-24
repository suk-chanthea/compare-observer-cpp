@echo off
REM Compare Observer - DEBUG BUILD with Console Output
setlocal enabledelayedexpansion

echo.
echo ========================================
echo  DEBUG BUILD - Console Mode
echo ========================================
echo.

REM Check for Qt installation
set "QtPath="
if exist "C:\Qt\6.10.1\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.1\msvc2022_64"
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.0\msvc2022_64"
) else (
    echo ERROR: Qt MSVC not found!
    pause
    exit /b 1
)

echo Qt path: %QtPath%
echo.

REM Find Visual Studio (2026 or 2022)
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
) else (
    echo ERROR: Visual Studio not found!
    pause
    exit /b 1
)

REM Initialize Visual Studio
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64

if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize Visual Studio
    pause
    exit /b 1
)

REM Create build_debug directory
if exist build_debug (
    rmdir /s /q build_debug
)
mkdir build_debug
cd build_debug

REM Configure with Debug mode
echo.
echo Configuring for DEBUG with console output...
cmake -G "%VS_GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="%QtPath%" ..

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build Debug
echo.
echo Building DEBUG version...
cmake --build . --config Debug

if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    cd ..
    pause
    exit /b 1
)

REM Find the executable
set "EXE_PATH="
if exist "bin\Debug\CompareObserver.exe" (
    set "EXE_PATH=bin\Debug\CompareObserver.exe"
) else if exist "bin\CompareObserver.exe" (
    set "EXE_PATH=bin\CompareObserver.exe"
) else if exist "Debug\CompareObserver.exe" (
    set "EXE_PATH=Debug\CompareObserver.exe"
) else (
    echo ERROR: Executable not found!
    echo Searching...
    dir /s /b CompareObserver.exe
    cd ..
    pause
    exit /b 1
)

echo Found executable: %EXE_PATH%
echo.

REM Deploy Qt DLLs
echo Deploying Qt DLLs...
"%QtPath%\bin\windeployqt.exe" --debug "%EXE_PATH%"

echo.
echo ========================================
echo  DEBUG BUILD COMPLETE!
echo ========================================
echo.
echo Launching with console output...
echo Any errors will be displayed in this window.
echo.
pause

REM Run with console visible (staying in build_debug directory)
echo Starting application...
echo.
"%EXE_PATH%"

echo.
echo ========================================
echo Application closed. Check above for any error messages.
echo ========================================
pause