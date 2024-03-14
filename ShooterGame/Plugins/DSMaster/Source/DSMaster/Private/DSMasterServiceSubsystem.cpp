// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMasterServiceSubsystem.h"

bool UDSMasterServiceSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}


bool UDSMasterServiceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	FString DSMasterName;

	if (!Super::ShouldCreateSubsystem(Outer))
		return false;
	
	// Run as Master Server
	if (FParse::Value(FCommandLine::Get(), TEXT("Master="), DSMasterName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Master")))
	{
		return true;
	}

	// Run as Agent Server
	if (FParse::Value(FCommandLine::Get(), TEXT("Agent="), DSMasterName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Agent")))
	{
		return true;
	}

	return false;
}

void UDSMasterServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UDSMasterServiceSubsystem::OnWorldInitialized);
		World->OnWorldBeginPlay.AddUObject(this, &UDSMasterServiceSubsystem::OnWorldBeginPlay);
		FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UDSMasterServiceSubsystem::OnBeginTearingDown);
	}
}

void UDSMasterServiceSubsystem::Deinitialize()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->OnWorldBeginPlay.RemoveAll(this);
	}
	
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
}

void UDSMasterServiceSubsystem::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
}

void UDSMasterServiceSubsystem::OnWorldBeginPlay()
{
	DSMasterService = MakeShared<FDSMasterService>();
	if (DSMasterService)
	{
		DSMasterService->InitServer();
		DSMasterService->StartDSMasterHttpService();
		DSMasterService->StartDSMasterServer(GetWorld());
	}
}

void UDSMasterServiceSubsystem::OnBeginTearingDown(UWorld* World)
{
	if (DSMasterService)
	{
		DSMasterService->StopServer();
		DSMasterService = nullptr;
	}
}

