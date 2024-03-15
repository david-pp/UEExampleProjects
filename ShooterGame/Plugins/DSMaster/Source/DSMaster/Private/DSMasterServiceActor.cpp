// Fill out your copyright notice in the Description page of Project Settings.

#include "DSMasterServiceActor.h"

void ADSMasterServiceActor::BeginPlay()
{
	Super::BeginPlay();

	DSMasterService = MakeShared<FDSMasterService>();
    if (DSMasterService)
    {
    	DSMasterService->InitServer(true);
    	DSMasterService->StartDSMasterHttpService();
    	DSMasterService->StartDSMasterServer(GetWorld());
    }
}

void ADSMasterServiceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (DSMasterService)
	{
		DSMasterService->StopServer();
		DSMasterService = nullptr;
	}
}
