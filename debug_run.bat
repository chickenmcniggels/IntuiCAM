@echo off
echo Starting IntuiCAM Debug Application...
echo =====================================

rem Set Qt logging environment variable to capture debug output
set QT_LOGGING_RULES=*=true

rem Set Qt debug output to console
set QT_QPA_PLATFORM_PLUGIN_PATH=%~dp0build_vs\Debug

rem Run the application and capture output
echo Starting application at %TIME%
"%~dp0build_vs\Debug\IntuiCAMGui.exe" 2>&1

echo Application finished at %TIME%
echo Return code: %ERRORLEVEL%

rem Wait for user input so we can see the output
pause 