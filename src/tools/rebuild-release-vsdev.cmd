@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2026\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x64
if errorlevel 1 exit /b %errorlevel%
cmake --build D:\CPP_project\NeurolingsCE\out\build\x64-Release --config Release
exit /b %errorlevel%
