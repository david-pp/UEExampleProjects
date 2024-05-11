// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "HelloGameUserSettings.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class HELLOUE_API UHelloGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	// ~Begin UGameUserSettings
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	virtual void SetToDefaults() override;
	// ~End UGameUserSettings

	UFUNCTION(BlueprintCallable, Category=Settings)
	void SetMusicVolume(float VolumeRate)
	{
		MusicVolume = VolumeRate;
	}

	UFUNCTION(BlueprintPure, Category = Settings)
	float GetMusicVolume() const
	{
		return MusicVolume;
	}

protected:
	UPROPERTY(config)
	float MusicVolume = 1.0f;
};
