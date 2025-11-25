@echo off
REM Compare Observer - DIAGNOSTIC BUILD
setlocal enabledelayedexpansion

echo.
echo ========================================
echo  DIAGNOSTIC BUILD - Full Debug Mode
echo ========================================
echo.

REM Step 1: Find Qt
set "QtPath="
if exist "C:\Qt\6.10.1\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.1\msvc2022_64"
    echo [OK] Found Qt 6.10.1
) else if exist "C:\Qt\6.10.0\msvc2022_64" (
    set "QtPath=C:\Qt\6.10.0\msvc2022_64"
    echo [OK] Found Qt 6.10.0
) else (
    echo [ERROR] Qt not found in expected locations!
    echo Please check if Qt is installed in C:\Qt\
    pause
    exit /b 1
)

REM Step 2: Find Visual Studio
set "VSDIR="
set "VS_GENERATOR="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Community"
    set "VS_GENERATOR=Visual Studio 18 2026"
    echo [OK] Found Visual Studio 2026 Community
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VSDIR=C:\Program Files\Microsoft Visual Studio\2022\Community"
    set "VS_GENERATOR=Visual Studio 17 2022"
    echo [OK] Found Visual Studio 2022 Community
) else (
    echo [ERROR] Visual Studio not found!
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Step 3: Initialize Visual Studio
echo ========================================
call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" x64
if %errorlevel% neq 0 (
    echo [ERROR] Failed to initialize Visual Studio
    pause
    exit /b 1
)
echo [OK] Visual Studio initialized

echo.
echo ========================================
echo  Step 4: Clean Build Directory
echo ========================================
if exist build_debug (
    echo Removing old build directory...
    rmdir /s /q build_debug
)
mkdir build_debug
cd build_debug
echo [OK] Build directory created

echo.
echo ========================================
echo  Step 5: CMake Configuration
echo ========================================
cmake -G "%VS_GENERATOR%" -A x64 ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_PREFIX_PATH="%QtPath%" ^
    ..

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    echo.
    echo Possible issues:
    echo - CMakeLists.txt has errors
    echo - Qt path is incorrect
    echo - Missing CMake
    cd ..
    pause
    exit /b 1
)
echo [OK] CMake configuration successful

echo.
echo ========================================
echo  Step 6: Build Project
echo ========================================
cmake --build . --config Debug --verbose

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    echo.
    echo Check the output above for compilation errors
    cd ..
    pause
    exit /b 1
)
echo [OK] Build successful

echo.
echo ========================================
echo  Step 7: Locate Executable
echo ========================================
set "EXE_PATH="
if exist "bin\Debug\CompareObserver.exe" (
    set "EXE_PATH=bin\Debug\CompareObserver.exe"
    echo [OK] Found: bin\Debug\CompareObserver.exe
) else if exist "Debug\CompareObserver.exe" (
    set "EXE_PATH=Debug\CompareObserver.exe"
    echo [OK] Found: Debug\CompareObserver.exe
) else (
    echo [ERROR] Executable not found!
    echo.
    echo Searching entire build directory...
    dir /s /b CompareObserver.exe
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Step 8: Deploy Qt Dependencies
echo ========================================
echo Deploying Qt DLLs to executable directory...
"%QtPath%\bin\windeployqt.exe" --debug --verbose 1 "%EXE_PATH%"

if %errorlevel% neq 0 (
    echo [WARNING] windeployqt failed, but continuing...
) else (
    echo [OK] Qt DLLs deployed successfully
)

REM Get the directory of the executable
for %%F in ("%EXE_PATH%") do set "EXE_DIR=%%~dpF"

echo.
echo ========================================
echo  Step 9: Check OpenSSL
echo ========================================
if exist "C:\Program Files\OpenSSL-Win64\bin\libssl-3-x64.dll" (
    echo Copying OpenSSL DLLs...
    copy /Y "C:\Program Files\OpenSSL-Win64\bin\libssl-3-x64.dll" "%EXE_DIR%"
    copy /Y "C:\Program Files\OpenSSL-Win64\bin\libcrypto-3-x64.dll" "%EXE_DIR%"
    echo [OK] OpenSSL DLLs copied
) else (
    echo [WARNING] OpenSSL not found at standard location
    echo Network features may not work
)

echo.
echo ========================================
echo  Step 10: Verify DLLs
echo ========================================
echo Checking for required Qt DLLs in %EXE_DIR%...
set "MISSING_DLLS="

if not exist "%EXE_DIR%Qt6Core.dll" set "MISSING_DLLS=!MISSING_DLLS! Qt6Core.dll"
if not exist "%EXE_DIR%Qt6Gui.dll" set "MISSING_DLLS=!MISSING_DLLS! Qt6Gui.dll"
if not exist "%EXE_DIR%Qt6Widgets.dll" set "MISSING_DLLS=!MISSING_DLLS! Qt6Widgets.dll"
if not exist "%EXE_DIR%Qt6Network.dll" set "MISSING_DLLS=!MISSING_DLLS! Qt6Network.dll"

if defined MISSING_DLLS (
    echo [WARNING] Missing DLLs:!MISSING_DLLS!
    echo Manually copying Qt DLLs...
    copy /Y "%QtPath%\bin\Qt6Core.dll" "%EXE_DIR%"
    copy /Y "%QtPath%\bin\Qt6Gui.dll" "%EXE_DIR%"
    copy /Y "%QtPath%\bin\Qt6Widgets.dll" "%EXE_DIR%"
    copy /Y "%QtPath%\bin\Qt6Network.dll" "%EXE_DIR%"
    copy /Y "%QtPath%\bin\Qt6Concurrent.dll" "%EXE_DIR%"
) else (
    echo [OK] All required Qt DLLs present
)

echo.
echo ========================================
echo  Step 11: Test Executable
echo ========================================
echo Testing if executable can be loaded...
where "%EXE_PATH%"
dumpbin /dependents "%EXE_PATH%" > dependencies.txt 2>&1
echo Dependencies saved to dependencies.txt

echo.
echo ========================================
echo  BUILD COMPLETE - READY TO RUN
echo ========================================
echo.
echo Executable: %EXE_PATH%
echo Log file: compare_observer_debug.log
echo.
echo The application will now start with full console output.
echo If it crashes, check the log file and error messages below.
echo.
pause

echo.
echo ========================================
echo  STARTING APPLICATION
echo ========================================
echo.

REM Change to the executable directory so it finds DLLs
pushd "%EXE_DIR%"

REM Run with error capturing
"%EXE_PATH%" 2>&1

set "EXIT_CODE=%errorlevel%"
popd

echo.
echo ========================================
echo  APPLICATION EXITED
echo ========================================
echo Exit code: %EXIT_CODE%
echo.

REM Display log file if it exists
if exist "%EXE_DIR%compare_observer_debug.log" (
    echo === LOG FILE CONTENTS ===
    type "%EXE_DIR%compare_observer_debug.log"
    echo.
    echo === END OF LOG FILE ===
) else (
    echo [WARNING] No log file created - application may have crashed before initialization
)

echo.
echo If the application didn't start:
echo 1. Check the error messages above
echo 2. Check dependencies.txt in the build directory
echo 3. Run: dumpbin /dependents "%EXE_PATH%"
echo 4. Try running from: %EXE_DIR%
echo.
pause