// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"


// Sets default values
AMyActor::AMyActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TotalDamage = 200;
	DamageTimeInSeconds = 1.0f;
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
	CalculateDPS();
	UE_LOG(LogTemp, Display, TEXT("创建一个AMyActor, TotalDamge=%d, DPS=%f"), TotalDamage, DamagePerSecond);
	UE_LOG(LogTemp, Display, TEXT("创建一个AMyActor, Name=%s"), *MyName.ToString());
}

void AMyActor::CalculateDPS()
{
	DamagePerSecond = TotalDamage / DamageTimeInSeconds;
}

void AMyActor::PostInitProperties()
{
	Super::PostInitProperties();
	CalculateDPS();
}

void AMyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	CalculateDPS();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AMyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	UE_LOG(LogTemp, Display, TEXT("删除一个AMyActor, TotalDamge=%d, DPS=%f"), TotalDamage, DamagePerSecond);
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

