// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ThirdPersonMP : ModuleRules
{
	public ThirdPersonMP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", 
				"Engine", 
				"InputCore",
				"Slate",
				"SlateCore",
				"UMG",
				"CommonUI",
				"HeadMountedDisplay", 
				"GameplayDebugger", 
				"AIModule",
				"GameplayTasks",
				"GameplayTags", 
				"OnlineSubsystemUtils",
				"OnlineSubsystemUtils",
				"Messaging",
				"MessagingCommon", 
				"MessagingRpc",
				"Networking", 
				"Serialization",
				"Sockets"
			});
	}
}