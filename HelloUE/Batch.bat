@echo off

REM The %~dp0 specifier resolves to the path to the directory where this .bat is located in.
set CurrentDir=%~dp0
echo %CurrentDir%

REM The %cd% System read-only variable %CD% keeps the path of the caller of the batch, not the batch file location.
set WorkingDir=%cd%
echo %WorkingDir%

:: current dir name
set path=%~p0
set path=%path:~0,-1%
For %%A in ("%path%") do (Set CurrDirName=%%~nxA)
echo %CurrDirName%

echo %UnrealEnginePath%
%UnrealEnginePath%\Engine\Binaries\DotNET\UnrealBuildTool.exe 


REM %1 is the first arg
REM %2 is the second arg
REM %* is all args

echo %1
echo %2
echo %*
