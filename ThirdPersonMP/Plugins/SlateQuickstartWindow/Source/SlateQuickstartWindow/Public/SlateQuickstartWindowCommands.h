// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SlateQuickstartWindowStyle.h"

class FSlateQuickstartWindowCommands : public TCommands<FSlateQuickstartWindowCommands>
{
public:

	FSlateQuickstartWindowCommands()
		: TCommands<FSlateQuickstartWindowCommands>(TEXT("SlateQuickstartWindow"), NSLOCTEXT("Contexts", "SlateQuickstartWindow", "SlateQuickstartWindow Plugin"), NAME_None, FSlateQuickstartWindowStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};