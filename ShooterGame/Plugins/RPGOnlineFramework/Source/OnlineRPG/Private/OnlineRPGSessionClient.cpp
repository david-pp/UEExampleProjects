// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGSessionClient.h"

UOnlineRPGSessionClient::UOnlineRPGSessionClient()
{
}

void UOnlineRPGSessionClient::OnSessionUserInviteAccepted(
	const bool							bWasSuccess,
	const int32							ControllerId,
	TSharedPtr< const FUniqueNetId > 	UserId,
	const FOnlineSessionSearchResult &	InviteResult
)
{
	UE_LOG(LogOnline, Verbose, TEXT("HandleSessionUserInviteAccepted: bSuccess: %d, ControllerId: %d, User: %s"), bWasSuccess, ControllerId, UserId.IsValid() ? *UserId->ToString() : TEXT("NULL"));

	if (!bWasSuccess)
	{
		return;
	}

	if (!InviteResult.IsSessionInfoValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no search result."));
		return;
	}

	if (!UserId.IsValid())
	{
		UE_LOG(LogOnline, Warning, TEXT("Invite accept returned no user."));
		return;
	}
	//
	// UOnlineRPGGameInstance* OnlineRPGGameInstance = Cast<UOnlineRPGGameInstance>(GetGameInstance());
	//
	// if (OnlineRPGGameInstance)
	// {
	// 	FOnlineRPGPendingInvite PendingInvite;
	//
	// 	// Set the pending invite, and then go to the initial screen, which is where we will process it
	// 	PendingInvite.ControllerId = ControllerId;
	// 	PendingInvite.UserId = UserId;
	// 	PendingInvite.InviteResult = InviteResult;
	// 	PendingInvite.bPrivilegesCheckedAndAllowed = false;
	//
	// 	OnlineRPGGameInstance->SetPendingInvite(PendingInvite);
	// 	OnlineRPGGameInstance->GotoState(OnlineRPGGameInstanceState::PendingInvite);
	// }
}