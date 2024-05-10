@echo off
:: %1 is the program name
:: The %~dp0 specifier resolves to the path to the directory where this .bat is located in.
:: The %cd% System read-only variable %CD% keeps the path of the caller of the batch, not the batch file location.

set ProgramName=%1
:: set UnrealEnginePath="D:\UnrealEngine427"
%UnrealEnginePath%\Engine\Binaries\DotNET\UnrealBuildTool.exe %ProgramName% Win64 Development -Project="%~dp0\HelloUE.uproject"
