@echo off
REM BOOM - Windows Batch Installer
REM Usage: Download and run boom_install.bat

echo.
echo   ███████╗  ██████╗ ███████╗
echo   ██╔════╝ ██╔═══██╗██╔════╝
echo   ███████╗ ██║   ██║███████╗
echo   ╚════██║ ██║   ██║╚════██║
echo   ███████║ ╚██████╔╝███████║
echo   ╚══════╝  ╚═════╝ ╚══════╝
echo   Terminal DOOM Clone v1.0.0
echo.
echo   https://github.com/Xznder1984/BOOM
echo.

set BOOM_DIR=%USERPROFILE%\.boom

echo Checking for Python...
set PYTHON=
where python >nul 2>&1 && set PYTHON=python
if "%PYTHON%"=="" where python3 >nul 2>&1 && set PYTHON=python3
if "%PYTHON%"=="" where py >nul 2>&1 && set PYTHON=py

if "%PYTHON%"=="" (
    echo.
    echo   ERROR: Python 3.6+ not found!
    echo.
    echo   Please install Python from: https://www.python.org/downloads/
    echo   Make sure to check 'Add Python to PATH'!
    echo.
    pause
    exit /b 1
)

echo   Found Python: %PYTHON%

echo Checking for Git...
set HAS_GIT=
where git >nul 2>&1 && set HAS_GIT=1

echo.
echo Downloading BOOM...

if exist "%BOOM_DIR%" rmdir /s /q "%BOOM_DIR%" 2>nul

if defined HAS_GIT (
    echo   Cloning repository...
    git clone --depth 1 https://github.com/Xznder1984/BOOM.git "%BOOM_DIR%"
) else (
    echo   Git not found. Please install Git or use the PowerShell installer:
    echo   powershell -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.ps1 | iex"
    pause
    exit /b 1
)

if not exist "%BOOM_DIR%\boom.py" (
    echo   Download failed!
    pause
    exit /b 1
)

echo   Installing dependencies...
%PYTHON% -m pip install --upgrade pip 2>nul
%PYTHON% -m pip install windows-curses 2>nul

echo.
echo ========================================
echo   BOOM installed to %BOOM_DIR%
echo ========================================
echo.
echo   Run: %PYTHON% %BOOM_DIR%\boom.py
echo   Or:  cd %BOOM_DIR% ^& %PYTHON% boom.py
echo.
echo   With DOOM WAD: %PYTHON% %BOOM_DIR%\boom.py --wad doom1.wad
echo.
echo Controls:
echo   W/A/S/D = Move    Left/Right = Turn
echo   F = Fire           Space = Use
echo   1-7 = Weapons      Tab = Map
echo   Q = Quit
echo.
pause
