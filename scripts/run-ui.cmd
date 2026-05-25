@echo off
setlocal

cd /d "%~dp0.."

call scripts\build-ui.cmd
if errorlevel 1 exit /b %ERRORLEVEL%

set "CLASSES_DIR=kraken-ui\build\classes\java\main"
set "UI_NATIVE=cmake-build-debug\ui-native\Debug\kraken-ui-native.dll"

if exist "%UI_NATIVE%" (
    java -Dkraken.ui.native="%CD%\%UI_NATIVE%" -cp "%CLASSES_DIR%" com.kraken.ui.KrakenLauncherApp %*
) else (
    java -cp "%CLASSES_DIR%" com.kraken.ui.KrakenLauncherApp %*
)
