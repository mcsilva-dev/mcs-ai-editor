@echo off
rem Abre o ambiente MSVC x64 (vcvars64) e executa o comando recebido.
rem Uso: scripts\dev.cmd cmake --preset debug
setlocal
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
  echo Visual Studio com C++ nao encontrado.
  exit /b 1
)
if not defined VCPKG_ROOT set "VCPKG_ROOT=%VSDIR%\VC\vcpkg"
if not defined QT_ROOT_DIR set "QT_ROOT_DIR=C:\Qt\6.11.3\msvc2022_64"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
%*
