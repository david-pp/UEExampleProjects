@echo off
:: set UnrealEnginePath="D:\UnrealEngine427"
:: set current director name to ProgramName
set CurentPath=%~p0
set CurentPath=%CurentPath:~0,-1%
For %%A in ("%CurentPath%") do (Set ProgramName=%%~nxA)

set "Platform=Win64"
set "Configuration=Development"
set "UnrealBuildTool=%UnrealEnginePath%\Engine\Binaries\DotNET\UnrealBuildTool.exe"

if {%1}=={}  (
    set "ProjectFile=%~dp0\..\..\..\HelloUE.uproject"
) else (
    set "ProjectFile=%1"
)

echo %UnrealEnginePath%
echo %UnrealBuildTool%
echo %ProgramName%
echo %ProjectFile%
 
%UnrealBuildTool% %ProgramName% %Platform% %Configuration% -Project=%ProjectFile%