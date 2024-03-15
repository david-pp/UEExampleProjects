// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSessionClient.h"
#include "OnlineRPGSessionClient.generated.h"

UCLASS(Config = Game)
class ONLINERPG_API UOnlineRPGSessionClient : public UOnlineSessionClient
{
	GENERATED_BODY()

public:
	/** Ctor */
	UOnlineRPGSessionClient();

	virtual void OnSessionUserInviteAccepted(
		const bool							bWasSuccess,
		const int32							ControllerId,
		TSharedPtr< const FUniqueNetId >	UserId,
		const FOnlineSessionSearchResult &	InviteResult
	) override;

};
