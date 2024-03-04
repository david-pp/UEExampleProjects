
::Run DSMaster Server
@echo on

echo "Starting DSMaster... "
start .\Binaries\Win64\ShooterServer-Win64-Debug.exe DSMasterEntry -Server -Log -Port=8000 -DSMaster=Manager -BeaconPort=9000


echo "Starting DSAgent1..."
start .\Binaries\Win64\ShooterServer-Win64-Debug.exe DSMasterEntry -Server -Log -Port=8001 -DSMaster=Agent -BeaconPort=9001 -DSMasterServer="127.0.0.1:9000"

echo "Starting DSAgent2..."
::start .\Binaries\Win64\ShooterServer-Win64-Debug.exe Sanctuary?game=/Script/ShooterGame.DSMasterGameMode -Server -Log -Port=8002 -DSMaster=Agent -BeaconPort=9002 -DSMasterServer="127.0.0.1:9000"