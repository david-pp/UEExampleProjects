// Fill out your copyright notice in the Description page of Project Settings.


#include "MyComponent.h"

#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UMyComponent::UMyComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UMyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UMyComponent, MyName, COND_None);
	DOREPLIFETIME_CONDITION_NOTIFY(UMyComponent, StringValue, COND_None, REPNOTIFY_OnChanged);
	DOREPLIFETIME(UMyComponent, StringArray);
	
}


// Called when the game starts
void UMyComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UMyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMyComponent::OnRep_StringValue(FString& OldValue)
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_StringValue@%d: %s -> %s"), GetOwnerRole(), *OldValue, *StringValue);
}

void UMyComponent::OnRep_StringArray(TArray<FString>& OldValues)
{
	UE_LOG(LogTemp, Warning, TEXT("OnRep_StringValue@%d: %s -> %s"), GetOwnerRole(), *FString::Join(OldValues, TEXT(",")), *FString::Join(StringArray, TEXT(",")));
}
