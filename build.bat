@echo off
setlocal
cd /d "%~dp0"
echo [Omni] building in %CD%

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ was not found in PATH.
    echo Install MinGW-w64 ^(x86_64, e.g. from winlibs.com or MSYS2 mingw64^) and add its "bin" folder to PATH.
    echo Then open a NEW terminal and run build.bat again.
    pause
    exit /b 1
)

g++ --version
echo.
echo [Omni] compiling...
g++ -O2 -std=c++17 -Wall -Wno-unused-function src\main.cpp src\window.cpp src\state.cpp src\util.cpp src\gfx.cpp src\json.cpp src\offsets.cpp src\process.cpp src\memory.cpp src\classmeta.cpp src\icons.cpp src\tree.cpp src\props.cpp src\live.cpp src\layout.cpp src\paint.cpp src\menus.cpp src\connection.cpp -o OmniExp.exe -mwindows -lcomctl32 -luxtheme -ldwmapi -lgdi32 -luser32 -lkernel32 > build.log 2>&1
set RC=%errorlevel%
type build.log
echo.
if not "%RC%"=="0" (
    echo [FAILED] exit code %RC%  ^(full log saved in build.log^)
) else (
    echo [OK] OmniExp.exe created
)
pause
exit /b %RC%
