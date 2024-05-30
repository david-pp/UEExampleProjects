#include "GameplayDebuggerExtension_CharacterSelection.h"

#include "GameplayDebuggerCategoryReplicator.h"
#include "Kismet/GameplayStatics.h"

GameplayDebuggerExtension_CharacterSelection::GameplayDebuggerExtension_CharacterSelection()
{
	BindKeyPress(EKeys::RightBracket.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &GameplayDebuggerExtension_CharacterSelection::OnDebugNextPawn, EGameplayDebuggerInputMode::Replicated);
	BindKeyPress(EKeys::LeftBracket.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &GameplayDebuggerExtension_CharacterSelection::OnDebugPrevPawn, EGameplayDebuggerInputMode::Replicated);
	const FGameplayDebuggerInputHandlerConfig KeyConfig(TEXT("Enter"), EKeys::Enter.GetFName());
	BindKeyPress(KeyConfig, this, &GameplayDebuggerExtension_CharacterSelection::TogglePlayerSelect);
}

void GameplayDebuggerExtension_CharacterSelection::OnActivated()
{
	FGameplayDebuggerExtension::OnActivated();
}

void GameplayDebuggerExtension_CharacterSelection::OnDeactivated()
{
	FGameplayDebuggerExtension::OnDeactivated();
}

FString GameplayDebuggerExtension_CharacterSelection::GetDescription() const
{
	return TEXT("CharacterSelection");
}

TSharedRef<FGameplayDebuggerExtension> GameplayDebuggerExtension_CharacterSelection::MakeInstance()
{
	return MakeShared<GameplayDebuggerExtension_CharacterSelection>();
}

void GameplayDebuggerExtension_CharacterSelection::TogglePlayerSelect()
{
	// Gets Replicator and updates the current debug actor based on whether there is already a valid selected player or not
	AGameplayDebuggerCategoryReplicator* Replicator = GetReplicator();
	if (Replicator)
	{
		APawn* Character = SelectedPlayer.Get();
		AActor* SelectedActor = InitialActor.Get();
		if (Character)
		{
			Replicator->SetDebugActor(SelectedActor, false);
			SelectedPlayer = nullptr;
		}
		else
		{
			APlayerController* OwnerPC = GetPlayerController();
			if (OwnerPC)
			{
				Character = OwnerPC->GetPawn();
				SelectedPlayer = Character;
				Replicator->SetDebugActor(Character, false);
			}
		}
	}
}

void GameplayDebuggerExtension_CharacterSelection::OnDebugNextPawn()
{
	// run at client
	APlayerController* OwnerPC = GetPlayerController();
	if (OwnerPC)
	{
		TArray<AActor*> Pawns;
		UGameplayStatics::GetAllActorsOfClass(OwnerPC, APawn::StaticClass(), Pawns);

		if (Pawns.Num() > 0)
		{
			CurrentTargetIndex++;
			CurrentTargetIndex %= Pawns.Num();
		}

		if (Pawns.IsValidIndex(CurrentTargetIndex))
		{
			AGameplayDebuggerCategoryReplicator* Replicator = GetReplicator();
			if (Replicator)
			{
				Replicator->SetDebugActor(Pawns[CurrentTargetIndex]);
			}
		}
	}
}

void GameplayDebuggerExtension_CharacterSelection::OnDebugPrevPawn()
{
	// run at client
	APlayerController* OwnerPC = GetPlayerController();
	if (OwnerPC)
	{
		TArray<AActor*> Pawns;
		UGameplayStatics::GetAllActorsOfClass(OwnerPC, APawn::StaticClass(), Pawns);

		if (Pawns.Num() > 0)
		{
			if (CurrentTargetIndex > 0)
			{
				CurrentTargetIndex--;
			}
			else
			{
				CurrentTargetIndex = Pawns.Num() - 1;
			}
		}

		if (Pawns.IsValidIndex(CurrentTargetIndex))
		{
			AGameplayDebuggerCategoryReplicator* Replicator = GetReplicator();
			if (Replicator)
			{
				Replicator->SetDebugActor(Pawns[CurrentTargetIndex]);
			}
		}
	}
}
