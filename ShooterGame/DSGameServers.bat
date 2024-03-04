

echo Run DS1
start .\Binaries\Win64\ShooterServer-Win64-Debug.exe Sanctuary -Log -Port=7778 -DSAgentServer="127.0.0.1:9001"

echo Run DS2
start .\Binaries\Win64\ShooterServer-Win64-Debug.exe HighRise -Log -Port=7779 -DSAgentServer="127.0.0.1:9001"