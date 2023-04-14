// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterBTComponent.h"


// Sets default values for this component's properties
UCharacterBTComponent::UCharacterBTComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// SetIsReplicatedByDefault(true);
	// SetIsReplicated(true);

	// ...
}


// Called when the game starts
void UCharacterBTComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UCharacterBTComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
