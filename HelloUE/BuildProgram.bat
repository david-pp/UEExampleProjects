@echo off
set PROGRAME=%1
%UnrealEnginePath%\Engine\Binaries\DotNET\UnrealBuildTool.exe %PROGRAME% Win64 Development -Project="%cd%\HelloUE.uproject"

