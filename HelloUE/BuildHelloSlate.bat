REM E:\UnrealEngine-4.27.2\Engine\Build\BatchFiles\Build.bat  -Target=" HelloSlate Win64 Development -Project=\"E:\UEExampleProjects\HelloUE\HelloUE.uproject\"" -Target="ShaderCompileWorker Win64 Development -Quiet" -WaitMutex -FromMsBuild

set UBT=E:\UnrealEngine-4.27.2\Engine\Binaries\DotNET\UnrealBuildTool.exe
%UBT% HelloSlate Win64 Development -Project="%cd%\HelloUE.uproject"

