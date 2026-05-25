@echo off
setlocal
setlocal EnableDelayedExpansion

cd /d "%~dp0.."

set "CLASSES_DIR=kraken-ui\build\classes\java\main"
set "SOURCES_FILE=%TEMP%\kraken-ui-sources-%RANDOM%.txt"

if not exist "%CLASSES_DIR%" mkdir "%CLASSES_DIR%"

if exist "%SOURCES_FILE%" del "%SOURCES_FILE%"
for /r "kraken-ui\src\main\java" %%F in (*.java) do (
    set "SOURCE=%%F"
    set "SOURCE=!SOURCE:\=/!"
    echo "!SOURCE!">>"%SOURCES_FILE%"
)

javac --release 11 -encoding UTF-8 -d "%CLASSES_DIR%" @"%SOURCES_FILE%"
set "JAVAC_EXIT=%ERRORLEVEL%"

del "%SOURCES_FILE%" >nul 2>nul

if not "%JAVAC_EXIT%"=="0" exit /b %JAVAC_EXIT%

echo Kraken UI compiled to %CLASSES_DIR%
