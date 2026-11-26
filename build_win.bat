@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
  echo Visual Studio 2022 Build Tools not found.
  exit /b 1
)

if not exist SDL2.lib (
  echo Generating SDL2.lib from SDL2.dll...
  echo LIBRARY SDL2 > SDL2.def
  echo EXPORTS >> SDL2.def
  for /f "skip=19 tokens=4" %%a in ('dumpbin /exports SDL2.dll') do (
    if not "%%a"=="" echo %%a >> SDL2.def
  )
  lib /nologo /machine:x64 /def:SDL2.def /out:SDL2.lib >nul
  del /q SDL2.def SDL2.exp 2>nul
)

cl /nologo /O2 /W3 /Isrc\win_sdl2 /D_CRT_SECURE_NO_WARNINGS src\wildguns.c /Fe:WildGuns.exe /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup SDL2.lib comdlg32.lib user32.lib shell32.lib
if errorlevel 1 exit /b 1
del /q wildguns.obj 2>nul
echo Build OK: WildGuns.exe
