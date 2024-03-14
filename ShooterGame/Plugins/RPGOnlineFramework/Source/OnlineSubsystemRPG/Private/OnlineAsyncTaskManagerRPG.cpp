// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineAsyncTaskManagerRPG.h"

void FOnlineAsyncTaskManagerRPG::OnlineTick()
{
	check(RPGSubsystem);
	check(FPlatformTLS::GetCurrentThreadId() == OnlineThreadId || !FPlatformProcess::SupportsMultithreading());
}

