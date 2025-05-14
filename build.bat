@echo off
setlocal

:: Configure build variables
set BUILD_DIR=build
set INSTALL_DIR=install
set BUILD_TYPE=Debug
set GENERATOR="Visual Studio 17 2022"
set PLATFORM=x64

:: Set environment paths
set QT_DIR=C:\Qt\6.9.0\msvc2022_64
set OPENCASCADE_DIR=C:\OpenCASCADE\occt-vc144-64-with-debug
set OPENCASCADE_THIRD_PARTY_DIR=C:\OpenCASCADE\3rdparty-vc14-64

echo ================================================================
echo Building IntuiCAM with the following configuration:
echo Build Directory: %BUILD_DIR%
echo Install Directory: %INSTALL_DIR%
echo Build Type: %BUILD_TYPE%
echo Generator: %GENERATOR%
echo Platform: %PLATFORM%
echo Qt Directory: %QT_DIR%
echo OpenCASCADE Directory: %OPENCASCADE_DIR%
echo OpenCASCADE Third Party Directory: %OPENCASCADE_THIRD_PARTY_DIR%
echo ================================================================

:: Create build directory if it doesn't exist
if not exist %BUILD_DIR% (
    echo Creating build directory...
    mkdir %BUILD_DIR%
)

:: Create installation directory if it doesn't exist
if not exist %INSTALL_DIR% (
    echo Creating installation directory...
    mkdir %INSTALL_DIR%
)

:: Configure with CMake
echo Configuring project with CMake...
cmake -S . -B %BUILD_DIR% ^
    -G %GENERATOR% ^
    -A %PLATFORM% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX=%CD%\%INSTALL_DIR% ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%\lib\cmake;%OPENCASCADE_DIR%\cmake" ^
    -DOpenCASCADE_ROOT_DIR=%OPENCASCADE_DIR% ^
    -DOpenCASCADE_THIRD_PARTY_DIR=%OPENCASCADE_THIRD_PARTY_DIR%

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b %ERRORLEVEL%
)

:: Build the project
echo Building project...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

:: Install the project
echo Installing project...
cmake --install %BUILD_DIR% --config %BUILD_TYPE%

if %ERRORLEVEL% neq 0 (
    echo Installation failed!
    exit /b %ERRORLEVEL%
)

echo ================================================================
echo Build completed successfully!
echo Installation directory: %CD%\%INSTALL_DIR%
echo ================================================================

endlocal 