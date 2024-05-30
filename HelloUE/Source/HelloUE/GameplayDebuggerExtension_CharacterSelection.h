#pragma once

#include "GameplayDebuggerExtension.h"

class GameplayDebuggerExtension_CharacterSelection : public FGameplayDebuggerExtension
{
public:
	GameplayDebuggerExtension_CharacterSelection();

	virtual void OnActivated() override;
	virtual void OnDeactivated() override;
	virtual FString GetDescription() const override;

	static TSharedRef<FGameplayDebuggerExtension> MakeInstance();

protected:
	void TogglePlayerSelect();
	void OnDebugNextPawn();
	void OnDebugPrevPawn();
	// void OnShowNextCharacter();

	TWeakObjectPtr<APawn> SelectedPlayer;
	TWeakObjectPtr<APawn> InitialActor;

	int32 CurrentTargetIndex = 0;
};
