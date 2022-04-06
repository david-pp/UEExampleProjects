// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class HELLOUE_API AMyActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void CalculateDPS();

	// per-instance modified values
	virtual void PostInitProperties() override;

#ifdef WITH_EDITOR
virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HelloUE)
	int32 TotalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	float DamageTimeInSeconds;

	// 中间变量(Transient-不保存设置)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category=HelloUE)
	float DamagePerSecond;

	FName MyName;
};
