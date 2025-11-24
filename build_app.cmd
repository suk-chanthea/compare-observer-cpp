@echo off
REM Compare Observer - ENHANCED DEBUG BUILD
setlocal enabledelayedexpansion

echo.
echo ========================================
echo  ENHANCED DEBUG BUILD
echo ========================================
echo.

REM Find Qt
set "QtPath="
if exist "C:\Qt\6.10.1\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.1\msvc2022_64"
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.0\msvc2022_64"
) else (
    echo ERROR: Qt not found!
    pause
    exit /b 1
)

echo Qt: %QtPath%

REM Find Visual Studio
set "VSDIR="
set "VS_GENERATOR="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
    set "VS_GENERATOR=Visual Studio 18 2026"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Community"
    set "VS_GENERATOR=Visual Studio 17 2022"
) else (
    echo ERROR: Visual Studio not found!
    pause
    exit /b 1
)

echo VS: %VSDIR%
echo.

REM Initialize VS
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize VS
    pause
    exit /b 1
)

REM Clean and create build directory
if exist build_debug (
    rmdir /s /q build_debug
)
mkdir build_debug
cd build_debug

REM Configure
echo Configuring...
cmake -G "%VS_GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="%QtPath%" ..
if %errorlevel% neq 0 (
    echo ERROR: CMake failed
    cd ..
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    cd ..
    pause
    exit /b 1
)

REM Find executable
set "EXE_PATH="
if exist "bin\Debug\CompareObserver.exe" (
    set "EXE_PATH=bin\Debug\CompareObserver.exe"
) else if exist "Debug\CompareObserver.exe" (
    set "EXE_PATH=Debug\CompareObserver.exe"
) else (
    echo ERROR: Executable not found
    dir /s /b CompareObserver.exe
    cd ..
    pause
    exit /b 1
)

echo Found: %EXE_PATH%

REM Deploy Qt DLLs
echo Deploying Qt...
"%QtPath%\bin\windeployqt.exe" --debug "%EXE_PATH%"

REM Check OpenSSL
if exist "C:\Program Files\OpenSSL-Win64\bin\libssl-3-x64.dll" (
    echo Copying OpenSSL DLLs...
    copy "C:\Program Files\OpenSSL-Win64\bin\libssl-3-x64.dll" "bin\Debug\" 2>nul
    copy "C:\Program Files\OpenSSL-Win64\bin\libcrypto-3-x64.dll" "bin\Debug\" 2>nul
)

echo.
echo ========================================
echo  BUILD COMPLETE
echo ========================================
echo.
echo Starting application with debugging...
echo Log file: compare_observer_debug.log
echo Console will stay open to show errors.
echo.
pause

REM Run with full console output
echo --- APPLICATION OUTPUT STARTS ---
echo.
"%EXE_PATH%"
set "EXIT_CODE=%errorlevel%"

echo.
echo --- APPLICATION OUTPUT ENDS ---
echo.
echo Exit code: %EXIT_CODE%
echo.

REM Show log file if it exists
if exist "compare_observer_debug.log" (
    echo === LOG FILE CONTENTS ===
    type compare_observer_debug.log
    echo.
    echo === END OF LOG FILE ===
) else (
    echo No log file was created.
)

echo.
echo ========================================
pause