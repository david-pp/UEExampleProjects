// Fill out your copyright notice in the Description page of Project Settings.


#include "HelloGameUserSettings.h"

void UHelloGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);
	// TODO: change music volume
}

void UHelloGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MusicVolume = 1.0f;
}

