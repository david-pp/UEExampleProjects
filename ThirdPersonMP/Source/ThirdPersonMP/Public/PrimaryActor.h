// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrimaryActor.generated.h"

UCLASS()
class THIRDPERSONMP_API APrimaryActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APrimaryActor();

	static const FPrimaryAssetType PrimaryActorType;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
