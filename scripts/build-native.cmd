@echo off
setlocal

cd /d "%~dp0.."

cmake -S . -B cmake-build-debug -G "Visual Studio 17 2022"
if errorlevel 1 exit /b %ERRORLEVEL%

if not exist "cmake-build-debug\_deps\minhook-src\src\hde\hde64.c" (
    if exist "cmake-build-debug\_deps\minhook-subbuild\minhook-populate.vcxproj" (
        cmake --build cmake-build-debug\_deps\minhook-subbuild --target minhook-populate --config Debug
        if errorlevel 1 exit /b %ERRORLEVEL%
    )
)

cmake --build cmake-build-debug --target launcher --config Debug
if errorlevel 1 exit /b %ERRORLEVEL%

cmake --build cmake-build-debug --target kraken-ui-native --config Debug
if errorlevel 1 exit /b %ERRORLEVEL%

echo Native targets built successfully.
