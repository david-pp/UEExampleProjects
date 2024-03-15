// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGDemoSpectator.h"
#include "Engine/DemoNetDriver.h"

AOnlineRPGDemoSpectator::AOnlineRPGDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bShowMouseCursor = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void AOnlineRPGDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "InGameMenu", IE_Pressed, this, &AOnlineRPGDemoSpectator::OnToggleInGameMenu );

	InputComponent->BindAction( "NextWeapon", IE_Pressed, this, &AOnlineRPGDemoSpectator::OnIncreasePlaybackSpeed );
	InputComponent->BindAction( "PrevWeapon", IE_Pressed, this, &AOnlineRPGDemoSpectator::OnDecreasePlaybackSpeed );
}

void AOnlineRPGDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	// // Build menu only after game is initialized
	// OnlineRPGDemoPlaybackMenu = MakeShareable( new FOnlineRPGDemoPlaybackMenu() );
	// OnlineRPGDemoPlaybackMenu->Construct( Cast< ULocalPlayer >( Player ) );
	//
	// // Create HUD if this is playback
	// if (GetWorld() != nullptr && GetWorld()->GetDemoNetDriver() != nullptr && !GetWorld()->GetDemoNetDriver()->IsServer())
	// {
	// 	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	// 	{
	// 		DemoHUD = SNew(SOnlineRPGDemoHUD)
	// 			.PlayerOwner(this);
	//
	// 		GEngine->GameViewport->AddViewportWidgetContent(DemoHUD.ToSharedRef());
	// 	}
	// }
	//
	// FActorSpawnParameters SpawnInfo;
	//
	// SpawnInfo.Owner				= this;
	// SpawnInfo.Instigator		= GetInstigator();
	// SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//
	// PlaybackSpeed = 2;
	//
	// FInputModeGameAndUI InputMode;
	// InputMode.SetWidgetToFocus(DemoHUD);

	// SetInputMode(InputMode);
}

void AOnlineRPGDemoSpectator::OnToggleInGameMenu()
{
	// if no one's paused, pause
	// if ( OnlineRPGDemoPlaybackMenu.IsValid() )
	// {
	// 	OnlineRPGDemoPlaybackMenu->ToggleGameMenu();
	// }
}

static float PlaybackSpeedLUT[5] = { 0.1f, 0.5f, 1.0f, 2.0f, 4.0f };

void AOnlineRPGDemoSpectator::OnIncreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed + 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AOnlineRPGDemoSpectator::OnDecreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed - 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AOnlineRPGDemoSpectator::Destroyed()
{
	if (GEngine != nullptr && GEngine->GameViewport != nullptr && DemoHUD.IsValid())
	{
		// Remove HUD
		// GEngine->GameViewport->RemoveViewportWidgetContent(DemoHUD.ToSharedRef());
	}

	Super::Destroyed();
}
