@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%~dp0animation-studio\studio.py"
    goto done
)
where python >nul 2>nul
if not errorlevel 1 (
    python "%~dp0animation-studio\studio.py"
    goto done
)
echo Python 3 is required. Install it from https://www.python.org/downloads/
echo Then double-click this file again. No packages need to be installed.
:done
if errorlevel 1 pause
