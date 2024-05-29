#include "GameplayDebuggerExtension_CharacterSelection.h"

#include "GameplayDebuggerCategoryReplicator.h"

GameplayDebuggerExtension_CharacterSelection::GameplayDebuggerExtension_CharacterSelection()
{
	// BindKeyPress(EKeys::RightBracket.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &GameplayDebuggerExtension_CharacterSelection::OnShowNextAgent, EGameplayDebuggerInputMode::Replicated);
	// BindKeyPress(EKeys::LeftBracket.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &GameplayDebuggerExtension_CharacterSelection::OnRequestAvatarUpdate, EGameplayDebuggerInputMode::Replicated);
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
