// Copyright 2020 Dan Kestranek.

#include "GDEngineSubsystem.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagsManager.h"

struct NativeTagAdder
{
	NativeTagAdder(FName TheTagName, FString TheTagComment) : TagName(TheTagName), TagComment(TheTagComment)
	{
		UGameplayTagsManager::OnLastChanceToAddNativeTags().AddLambda([this]()
		{
			Tag = UGameplayTagsManager::Get().AddNativeGameplayTag(TagName, TagComment);
		});
	}

	FName TagName;
	FString TagComment;
	FGameplayTag Tag;
};

// 方式2：
static NativeTagAdder NativeTag_RPG_StateC(FName(TEXT("RPG.Native.StateC")), TEXT("StateC .."));
static NativeTagAdder NativeTag_RPG_StateD(FName(TEXT("RPG.Native.StateD")), TEXT("StateD .."));


void UGDEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 方式1：
	UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TEXT("RPG.Native.StateA")), TEXT("State A"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TEXT("RPG.Native.StateB")), TEXT("State B"));
	UGameplayTagsManager::Get().DoneAddingNativeTags();

	UAbilitySystemGlobals::Get().InitGlobalData();
}
