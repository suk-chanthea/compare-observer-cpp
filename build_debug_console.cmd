@echo off
REM Compare Observer - DEBUG BUILD with Console Output
REM Use this to see error messages when the window doesn't appear

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

echo Qt path: !QtPath!

REM Initialize Visual Studio
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

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

REM Configure with Debug mode and Console enabled
echo.
echo Configuring for DEBUG with console output...
cmake -DCMAKE_PREFIX_PATH="!QtPath!" ^
      -G "Visual Studio 18 2026" ^
      -A x64 ^
      -DCMAKE_BUILD_TYPE=Debug ^
      ..

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

REM Modify the project to show console
echo.
echo Modifying project for console output...
powershell -Command "(Get-Content 'CompareObserver.vcxproj') -replace 'WIN32_EXECUTABLE ON', 'WIN32_EXECUTABLE OFF' | Set-Content 'CompareObserver.vcxproj'"

REM Build Debug
echo.
echo Building DEBUG version...
cmake --build . --config Debug

if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

REM Deploy Qt DLLs
echo.
echo Deploying Qt DLLs...
"!QtPath!\bin\windeployqt.exe" --debug "bin\CompareObserver.exe"

echo.
echo ========================================
echo  DEBUG BUILD COMPLETE!
echo ========================================
echo.
echo Launching with console output...
echo Any errors will be displayed in this window.
echo.
pause

REM Run with console visible
cd bin
CompareObserver.exe

echo.
echo Application closed. Check above for any error messages.
pause