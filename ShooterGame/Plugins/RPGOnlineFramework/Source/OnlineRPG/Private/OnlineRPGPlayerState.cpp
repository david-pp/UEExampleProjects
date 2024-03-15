// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGPlayerState.h"

#include "OnlineRPGGameState.h"
#include "UnrealNetwork.h"
#include "Net/OnlineEngineInterface.h"

AOnlineRPGPlayerState::AOnlineRPGPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TeamNumber = 0;
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void AOnlineRPGPlayerState::Reset()
{
	Super::Reset();
	
	//PlayerStates persist across seamless travel.  Keep the same teams as previous match.
	//SetTeamNum(0);
	NumKills = 0;
	NumDeaths = 0;
	NumBulletsFired = 0;
	NumRocketsFired = 0;
	bQuitter = false;
}

void AOnlineRPGPlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	if (UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::RegisterPlayerWithSession(bWasFromInvite);
	}
}

void AOnlineRPGPlayerState::UnregisterPlayerWithSession()
{
	if (!IsFromPreviousLevel() && UOnlineEngineInterface::Get()->DoesSessionExist(GetWorld(), NAME_GameSession))
	{
		Super::UnregisterPlayerWithSession();
	}
}

void AOnlineRPGPlayerState::ClientInitialize(AController* InController)
{
	Super::ClientInitialize(InController);

	UpdateTeamColors();
}

void AOnlineRPGPlayerState::SetTeamNum(int32 NewTeamNumber)
{
	TeamNumber = NewTeamNumber;

	UpdateTeamColors();
}

void AOnlineRPGPlayerState::OnRep_TeamColor()
{
	UpdateTeamColors();
}

void AOnlineRPGPlayerState::AddBulletsFired(int32 NumBullets)
{
	NumBulletsFired += NumBullets;
}

void AOnlineRPGPlayerState::AddRocketsFired(int32 NumRockets)
{
	NumRocketsFired += NumRockets;
}

void AOnlineRPGPlayerState::SetQuitter(bool bInQuitter)
{
	bQuitter = bInQuitter;
}

void AOnlineRPGPlayerState::SetMatchId(const FString& CurrentMatchId)
{
	MatchId = CurrentMatchId;
}

void AOnlineRPGPlayerState::CopyProperties(APlayerState* PlayerState)
{	
	Super::CopyProperties(PlayerState);

	AOnlineRPGPlayerState* OnlineRPGPlayer = Cast<AOnlineRPGPlayerState>(PlayerState);
	if (OnlineRPGPlayer)
	{
		OnlineRPGPlayer->TeamNumber = TeamNumber;
	}	
}

void AOnlineRPGPlayerState::UpdateTeamColors()
{
	AController* OwnerController = Cast<AController>(GetOwner());
	if (OwnerController != NULL)
	{
		// AOnlineRPGCharacter* OnlineRPGCharacter = Cast<AOnlineRPGCharacter>(OwnerController->GetCharacter());
		// if (OnlineRPGCharacter != NULL)
		// {
		// 	OnlineRPGCharacter->UpdateTeamColorsAllMIDs();
		// }
	}
}

int32 AOnlineRPGPlayerState::GetTeamNum() const
{
	return TeamNumber;
}

int32 AOnlineRPGPlayerState::GetKills() const
{
	return NumKills;
}

int32 AOnlineRPGPlayerState::GetDeaths() const
{
	return NumDeaths;
}

int32 AOnlineRPGPlayerState::GetNumBulletsFired() const
{
	return NumBulletsFired;
}

int32 AOnlineRPGPlayerState::GetNumRocketsFired() const
{
	return NumRocketsFired;
}

bool AOnlineRPGPlayerState::IsQuitter() const
{
	return bQuitter;
}

FString AOnlineRPGPlayerState::GetMatchId() const
{
	return MatchId;
}

void AOnlineRPGPlayerState::ScoreKill(AOnlineRPGPlayerState* Victim, int32 Points)
{
	NumKills++;
	ScorePoints(Points);
}

void AOnlineRPGPlayerState::ScoreDeath(AOnlineRPGPlayerState* KilledBy, int32 Points)
{
	NumDeaths++;
	ScorePoints(Points);
}

void AOnlineRPGPlayerState::ScorePoints(int32 Points)
{
	AOnlineRPGGameState* const MyGameState = GetWorld()->GetGameState<AOnlineRPGGameState>();
	if (MyGameState && TeamNumber >= 0)
	{
		if (TeamNumber >= MyGameState->TeamScores.Num())
		{
			MyGameState->TeamScores.AddZeroed(TeamNumber - MyGameState->TeamScores.Num() + 1);
		}

		MyGameState->TeamScores[TeamNumber] += Points;
	}

	SetScore(GetScore() + Points);
}

void AOnlineRPGPlayerState::InformAboutKill_Implementation(class AOnlineRPGPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AOnlineRPGPlayerState* KilledPlayerState)
{
	//id can be null for bots
	if (KillerPlayerState->GetUniqueId().IsValid())
	{	
		//search for the actual killer before calling OnKill()	
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{		
			// AOnlineRPGPlayerController* TestPC = Cast<AOnlineRPGPlayerController>(*It);
			// if (TestPC && TestPC->IsLocalController())
			// {
			// 	// a local player might not have an ID if it was created with CreateDebugPlayer.
			// 	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(TestPC->Player);
			// 	FUniqueNetIdRepl LocalID = LocalPlayer->GetCachedUniqueNetId();
			// 	if (LocalID.IsValid() &&  *LocalPlayer->GetCachedUniqueNetId() == *KillerPlayerState->GetUniqueId())
			// 	{			
			// 		TestPC->OnKill();
			// 	}
			// }
		}
	}
}

void AOnlineRPGPlayerState::BroadcastDeath_Implementation(class AOnlineRPGPlayerState* KillerPlayerState, const UDamageType* KillerDamageType, class AOnlineRPGPlayerState* KilledPlayerState)
{	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		// all local players get death messages so they can update their huds.
		// AOnlineRPGPlayerController* TestPC = Cast<AOnlineRPGPlayerController>(*It);
		// if (TestPC && TestPC->IsLocalController())
		// {
		// 	TestPC->OnDeathMessage(KillerPlayerState, this, KillerDamageType);				
		// }
	}	
}

void AOnlineRPGPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRPGPlayerState, TeamNumber);
	DOREPLIFETIME(AOnlineRPGPlayerState, NumKills);
	DOREPLIFETIME(AOnlineRPGPlayerState, NumDeaths);
	DOREPLIFETIME(AOnlineRPGPlayerState, MatchId);
}

FString AOnlineRPGPlayerState::GetShortPlayerName() const
{
	// if( GetPlayerName().Len() > MAX_PLAYER_NAME_LENGTH )
	// {
	// 	return GetPlayerName().Left(MAX_PLAYER_NAME_LENGTH) + "...";
	// }
	return GetPlayerName();
}