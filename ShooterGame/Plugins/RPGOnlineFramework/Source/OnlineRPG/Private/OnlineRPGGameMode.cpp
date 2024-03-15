// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGGameMode.h"
#include "OnlineRPGSpectatorPawn.h"
#include "OnlineRPGDemoSpectator.h"

#include "EngineUtils.h"
#include "OnlineRPGPlayerState.h"
#include "OnlineRPGGameSession.h"
#include "OnlineRPGGameState.h"
#include "Engine/PlayerStartPIE.h"
#include "Kismet/GameplayStatics.h"


AOnlineRPGGameMode::AOnlineRPGGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnOb(TEXT("/Game/Blueprints/Pawns/PlayerPawn"));
	DefaultPawnClass = PlayerPawnOb.Class;
	
	static ConstructorHelpers::FClassFinder<APawn> BotPawnOb(TEXT("/Game/Blueprints/Pawns/BotPawn"));
	BotPawnClass = BotPawnOb.Class;

	// HUDClass = AOnlineRPGHUD::StaticClass();
	// PlayerControllerClass = AOnlineRPGPlayerController::StaticClass();
	PlayerStateClass = AOnlineRPGPlayerState::StaticClass();
	SpectatorClass = AOnlineRPGSpectatorPawn::StaticClass();
	// GameStateClass = AOnlineRPGGameState::StaticClass();
	ReplaySpectatorPlayerControllerClass = AOnlineRPGDemoSpectator::StaticClass();

	MinRespawnDelay = 5.0f;

	bAllowBots = true;	
	bNeedsBotCreation = true;
	bUseSeamlessTravel = FParse::Param(FCommandLine::Get(), TEXT("NoSeamlessTravel")) ? false : true;
}

void AOnlineRPGGameMode::PostInitProperties()
{
	Super::PostInitProperties();
	// if (PlatformPlayerControllerClass != nullptr)
	// {
	// 	PlayerControllerClass = PlatformPlayerControllerClass;
	// }
}

FString AOnlineRPGGameMode::GetBotsCountOptionName()
{
	return FString(TEXT("Bots"));
}

void AOnlineRPGGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	const int32 BotsCountOptionValue = UGameplayStatics::GetIntOption(Options, GetBotsCountOptionName(), 0);
	SetAllowBots(BotsCountOptionValue > 0 ? true : false, BotsCountOptionValue);	
	Super::InitGame(MapName, Options, ErrorMessage);

	// const UGameInstance* GameInstance = GetGameInstance();
	// if (GameInstance && Cast<UOnlineRPGGameInstance>(GameInstance)->GetOnlineMode() != EOnlineMode::Offline)
	// {
	// 	bPauseable = false;
	// }
}

void AOnlineRPGGameMode::SetAllowBots(bool bInAllowBots, int32 InMaxBots)
{
	bAllowBots = bInAllowBots;
	MaxBots = InMaxBots;
}

/** Returns game session class to use */
TSubclassOf<AGameSession> AOnlineRPGGameMode::GetGameSessionClass() const
{
	return AOnlineRPGGameSession::StaticClass();
}

void AOnlineRPGGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AOnlineRPGGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void AOnlineRPGGameMode::DefaultTimer()
{
	// don't update timers for Play In Editor mode, it's not real match
	if (GetWorld()->IsPlayInEditor())
	{
		// start match if necessary.
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			StartMatch();
		}
		return;
	}

	AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	if (MyGameState && MyGameState->RemainingTime > 0 && !MyGameState->bTimerPaused)
	{
		MyGameState->RemainingTime--;
		
		if (MyGameState->RemainingTime <= 0)
		{
			if (GetMatchState() == MatchState::WaitingPostMatch)
			{
				RestartGame();
			}
			else if (GetMatchState() == MatchState::InProgress)
			{
				FinishMatch();

				// Send end round events
				for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
				{
					// AOnlineRPGPlayerController* PlayerController = Cast<AOnlineRPGPlayerController>(*It);
					//
					// if (PlayerController && MyGameState)
					// {
					// 	AOnlineRPGPlayerState* PlayerState = Cast<AOnlineRPGPlayerState>((*It)->PlayerState);
					// 	const bool bIsWinner = IsWinner(PlayerState);
					//
					// 	PlayerController->ClientSendRoundEndEvent(bIsWinner, MyGameState->ElapsedTime);
					// }
				}
			}
			else if (GetMatchState() == MatchState::WaitingToStart)
			{
				StartMatch();
			}
		}
	}
}

void AOnlineRPGGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (bNeedsBotCreation)
	{
		CreateBotControllers();
		bNeedsBotCreation = false;
	}

	if (bDelayedStart)
	{
		// start warmup if needed
		AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
		if (MyGameState && MyGameState->RemainingTime == 0)
		{
			const bool bWantsMatchWarmup = !GetWorld()->IsPlayInEditor();
			if (bWantsMatchWarmup && WarmupTime > 0)
			{
				MyGameState->RemainingTime = WarmupTime;
			}
			else
			{
				MyGameState->RemainingTime = 0.0f;
			}
		}
	}
}

void AOnlineRPGGameMode::HandleMatchHasStarted()
{
	bNeedsBotCreation = true;
	Super::HandleMatchHasStarted();

	AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	MyGameState->RemainingTime = RoundTime;	
	StartBots();	

	// notify players
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		// AOnlineRPGPlayerController* PC = Cast<AOnlineRPGPlayerController>(*It);
		// if (PC)
		// {
		// 	PC->ClientGameStarted();
		// }
	}
}

void AOnlineRPGGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AOnlineRPGGameMode::FinishMatch()
{
	AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	if (IsMatchInProgress())
	{
		EndMatch();
		DetermineMatchWinner();		

		// notify players
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AOnlineRPGPlayerState* PlayerState = Cast<AOnlineRPGPlayerState>((*It)->PlayerState);
			const bool bIsWinner = IsWinner(PlayerState);

			(*It)->GameHasEnded(NULL, bIsWinner);
		}

		// lock all pawns
		// pawns are not marked as keep for seamless travel, so we will create new pawns on the next match rather than
		// turning these back on.
		for (APawn* Pawn : TActorRange<APawn>(GetWorld()))
		{
			Pawn->TurnOff();
		}

		// set up to restart the match
		MyGameState->RemainingTime = TimeBetweenMatches;
	}
}

void AOnlineRPGGameMode::RequestFinishAndExitToMainMenu()
{
	FinishMatch();

	// UOnlineRPGGameInstance* const GameInstance = Cast<UOnlineRPGGameInstance>(GetGameInstance());
	// if (GameInstance)
	// {
	// 	GameInstance->RemoveSplitScreenPlayers();
	// }
	//
	// AOnlineRPGPlayerController* LocalPrimaryController = nullptr;
	// for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	// {
	// 	AOnlineRPGPlayerController* Controller = Cast<AOnlineRPGPlayerController>(*Iterator);
	//
	// 	if (Controller == NULL)
	// 	{
	// 		continue;
	// 	}
	//
	// 	if (!Controller->IsLocalController())
	// 	{
	// 		const FText RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.");
	// 		Controller->ClientReturnToMainMenuWithTextReason(RemoteReturnReason);
	// 	}
	// 	else
	// 	{
	// 		LocalPrimaryController = Controller;
	// 	}
	// }
	//
	// // GameInstance should be calling this from an EndState.  So call the PC function that performs cleanup, not the one that sets GI state.
	// if (LocalPrimaryController != NULL)
	// {
	// 	LocalPrimaryController->HandleReturnToMainMenu();
	// }
}

void AOnlineRPGGameMode::DetermineMatchWinner()
{
	// nothing to do here
}

bool AOnlineRPGGameMode::IsWinner(class AOnlineRPGPlayerState* PlayerState) const
{
	return false;
}

void AOnlineRPGGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	const bool bMatchIsOver = MyGameState && MyGameState->HasMatchEnded();
	if( bMatchIsOver )
	{
		ErrorMessage = TEXT("Match is over!");
	}
	else
	{
		// GameSession can be NULL if the match is over
		// Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

		ErrorMessage = GameSession->ApproveLogin(Options);
		FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
	}
}


void AOnlineRPGGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// // update spectator location for client
	// AOnlineRPGPlayerController* NewPC = Cast<AOnlineRPGPlayerController>(NewPlayer);
	// if (NewPC && NewPC->GetPawn() == NULL)
	// {
	// 	NewPC->ClientSetSpectatorCamera(NewPC->GetSpawnLocation(), NewPC->GetControlRotation());
	// }
	//
	// // notify new player if match is already in progress
	// if (NewPC && IsMatchInProgress())
	// {
	// 	NewPC->ClientGameStarted();
	// 	NewPC->ClientStartOnlineGame();
	// }
}

void AOnlineRPGGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType)
{
	AOnlineRPGPlayerState* KillerPlayerState = Killer ? Cast<AOnlineRPGPlayerState>(Killer->PlayerState) : NULL;
	AOnlineRPGPlayerState* VictimPlayerState = KilledPlayer ? Cast<AOnlineRPGPlayerState>(KilledPlayer->PlayerState) : NULL;

	if (KillerPlayerState && KillerPlayerState != VictimPlayerState)
	{
		KillerPlayerState->ScoreKill(VictimPlayerState, KillScore);
		KillerPlayerState->InformAboutKill(KillerPlayerState, DamageType, VictimPlayerState);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->ScoreDeath(KillerPlayerState, DeathScore);
		VictimPlayerState->BroadcastDeath(KillerPlayerState, DamageType, VictimPlayerState);
	}
}

float AOnlineRPGGameMode::ModifyDamage(float Damage, AActor* DamagedActor, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	float ActualDamage = Damage;
	//
	// AOnlineRPGCharacter* DamagedPawn = Cast<AOnlineRPGCharacter>(DamagedActor);
	// if (DamagedPawn && EventInstigator)
	// {
	// 	AOnlineRPGPlayerState* DamagedPlayerState = Cast<AOnlineRPGPlayerState>(DamagedPawn->GetPlayerState());
	// 	AOnlineRPGPlayerState* InstigatorPlayerState = Cast<AOnlineRPGPlayerState>(EventInstigator->PlayerState);
	//
	// 	// disable friendly fire
	// 	if (!CanDealDamage(InstigatorPlayerState, DamagedPlayerState))
	// 	{
	// 		ActualDamage = 0.0f;
	// 	}
	//
	// 	// scale self instigated damage
	// 	if (InstigatorPlayerState == DamagedPlayerState)
	// 	{
	// 		ActualDamage *= DamageSelfScale;
	// 	}
	// }

	return ActualDamage;
}

bool AOnlineRPGGameMode::CanDealDamage(class AOnlineRPGPlayerState* DamageInstigator, class AOnlineRPGPlayerState* DamagedPlayer) const
{
	return true;
}

bool AOnlineRPGGameMode::AllowCheats(APlayerController* P)
{
	return true;
}

bool AOnlineRPGGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

UClass* AOnlineRPGGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// if (InController->IsA<AOnlineRPGAIController>())
	// {
	// 	return BotPawnClass;
	// }

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AOnlineRPGGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	// AOnlineRPGPlayerController* PC = Cast<AOnlineRPGPlayerController>(NewPlayer);
	// if (PC)
	// {
	// 	// Since initial weapon is equipped before the pawn is added to the replication graph, need to resend the notify so that it can be added as a dependent actor
	// 	AOnlineRPGCharacter* Character = Cast<AOnlineRPGCharacter>(PC->GetCharacter());
	// 	if (Character)
	// 	{
	// 		AOnlineRPGCharacter::NotifyEquipWeapon.Broadcast(Character, Character->GetWeapon());
	// 	}
	// 	
	// 	PC->ClientGameStarted();
	// }
}

AActor* AOnlineRPGGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> PreferredSpawns;
	TArray<APlayerStart*> FallbackSpawns;

	APlayerStart* BestStart = NULL;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* TestSpawn = *It;
		if (TestSpawn->IsA<APlayerStartPIE>())
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			BestStart = TestSpawn;
			break;
		}
		else
		{
			if (IsSpawnpointAllowed(TestSpawn, Player))
			{
				if (IsSpawnpointPreferred(TestSpawn, Player))
				{
					PreferredSpawns.Add(TestSpawn);
				}
				else
				{
					FallbackSpawns.Add(TestSpawn);
				}
			}
		}
	}

	
	if (BestStart == NULL)
	{
		if (PreferredSpawns.Num() > 0)
		{
			BestStart = PreferredSpawns[FMath::RandHelper(PreferredSpawns.Num())];
		}
		else if (FallbackSpawns.Num() > 0)
		{
			BestStart = FallbackSpawns[FMath::RandHelper(FallbackSpawns.Num())];
		}
	}

	return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

bool AOnlineRPGGameMode::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	// AOnlineRPGTeamStart* OnlineRPGSpawnPoint = Cast<AOnlineRPGTeamStart>(SpawnPoint);
	// if (OnlineRPGSpawnPoint)
	// {
	// 	AOnlineRPGAIController* AIController = Cast<AOnlineRPGAIController>(Player);
	// 	if (OnlineRPGSpawnPoint->bNotForBots && AIController)
	// 	{
	// 		return false;
	// 	}
	//
	// 	if (OnlineRPGSpawnPoint->bNotForPlayers && AIController == NULL)
	// 	{
	// 		return false;
	// 	}
	// 	return true;
	// }

	return false;
}

bool AOnlineRPGGameMode::IsSpawnpointPreferred(APlayerStart* SpawnPoint, AController* Player) const
{
	// ACharacter* MyPawn = Cast<ACharacter>((*DefaultPawnClass)->GetDefaultObject<ACharacter>());	
	// AOnlineRPGAIController* AIController = Cast<AOnlineRPGAIController>(Player);
	// if( AIController != nullptr )
	// {
	// 	MyPawn = Cast<ACharacter>(BotPawnClass->GetDefaultObject<ACharacter>());
	// }
	//
	// if (MyPawn)
	// {
	// 	const FVector SpawnLocation = SpawnPoint->GetActorLocation();
	// 	for (ACharacter* OtherPawn : TActorRange<ACharacter>(GetWorld()))
	// 	{
	// 		if (OtherPawn != MyPawn)
	// 		{
	// 			const float CombinedHeight = (MyPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * 2.0f;
	// 			const float CombinedRadius = MyPawn->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherPawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
	// 			const FVector OtherLocation = OtherPawn->GetActorLocation();
	//
	// 			// check if player start overlaps this pawn
	// 			if (FMath::Abs(SpawnLocation.Z - OtherLocation.Z) < CombinedHeight && (SpawnLocation - OtherLocation).Size2D() < CombinedRadius)
	// 			{
	// 				return false;
	// 			}
	// 		}
	// 	}
	// }
	// else
	// {
	// 	return false;
	// }
	
	return true;
}

void AOnlineRPGGameMode::CreateBotControllers()
{
	UWorld* World = GetWorld();
	int32 ExistingBots = 0;
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		// AOnlineRPGAIController* AIC = Cast<AOnlineRPGAIController>(*It);
		// if (AIC)
		// {
		// 	++ExistingBots;
		// }
	}

	// Create any necessary AIControllers.  Hold off on Pawn creation until pawns are actually necessary or need recreating.	
	int32 BotNum = ExistingBots;
	for (int32 i = 0; i < MaxBots - ExistingBots; ++i)
	{
		CreateBot(BotNum + i);
	}
}

AOnlineRPGAIController* AOnlineRPGGameMode::CreateBot(int32 BotNum)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = nullptr;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = nullptr;

	// UWorld* World = GetWorld();
	// AOnlineRPGAIController* AIC = World->SpawnActor<AOnlineRPGAIController>(SpawnInfo);
	// InitBot(AIC, BotNum);
	//
	// return AIC;
	return nullptr;
}

void AOnlineRPGGameMode::StartBots()
{
	// checking number of existing human player.
	UWorld* World = GetWorld();
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{		
		// AOnlineRPGAIController* AIC = Cast<AOnlineRPGAIController>(*It);
		// if (AIC)
		// {
		// 	RestartPlayer(AIC);
		// }
	}	
}

void AOnlineRPGGameMode::InitBot(AOnlineRPGAIController* AIController, int32 BotNum)
{	
	if (AIController)
	{
		// if (AIController->PlayerState)
		// {
		// 	FString BotName = FString::Printf(TEXT("Bot %d"), BotNum);
		// 	AIController->PlayerState->SetPlayerName(BotName);
		// }		
	}
}

void AOnlineRPGGameMode::RestartGame()
{
	// Hide the scoreboard too !
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		// AOnlineRPGPlayerController* PlayerController = Cast<AOnlineRPGPlayerController>(*It);
		// if (PlayerController != nullptr)
		// {
		// 	AOnlineRPGHUD* OnlineRPGHUD = Cast<AOnlineRPGHUD>(PlayerController->GetHUD());
		// 	if (OnlineRPGHUD != nullptr)
		// 	{
		// 		// Passing true to bFocus here ensures that focus is returned to the game viewport.
		// 		OnlineRPGHUD->ShowScoreboard(false, true);
		// 	}
		// }
	}

	Super::RestartGame();
}

