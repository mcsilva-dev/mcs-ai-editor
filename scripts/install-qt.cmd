@echo off
rem Instala Qt 6.11.3 (msvc2022_64) em C:\Qt usando aqtinstall num venv isolado.
setlocal
set "QT_VER=6.11.3"
set "VENV=%LOCALAPPDATA%\mcs-tools\aqt-venv"
if exist "C:\Qt\%QT_VER%\msvc2022_64\bin\Qt6Core.dll" (
  echo Qt %QT_VER% ja instalado.
  exit /b 0
)
if not exist "%VENV%\Scripts\python.exe" py -3 -m venv "%VENV%" || exit /b 1
rem aqtinstall do GitHub: PyPI (3.3.0) nao suporta o layout de repo do Qt 6.11+ (PR #1000).
"%VENV%\Scripts\python.exe" -m pip install --quiet "aqtinstall @ git+https://github.com/miurahr/aqtinstall.git@076e1659807d0b362a3ed684d54c2e9c775eb9c7" || exit /b 1
"%VENV%\Scripts\python.exe" -m aqt install-qt windows desktop %QT_VER% win64_msvc2022_64 -O C:\Qt || exit /b 1
echo Qt %QT_VER% instalado em C:\Qt\%QT_VER%\msvc2022_64
