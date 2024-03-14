// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSessionTypes.h"
#include "OnlineSessionSettings.h"
// #include "OnlineSessionHelper.generated.h"

struct FOnlineSessionHelper
{
	//
	// Helper Functions
	//

	// OnlineSession -> SessionDetails
	static void SetupHttpSessionDetails(FGameSessionDetails& SessionDetails, FNamedOnlineSession* OnlineSession);
	// SessionDetails -> OnlineSession
	static void SetupOnlineSession(FOnlineSession* OnlineSession, const FGameSessionDetails& SessionDetails);

	// OnlineSessionSearch -> Param1=X&Param2=Y&...
	static FString SessionSearchToQueryParams(const FOnlineSessionSearch* Search);

};