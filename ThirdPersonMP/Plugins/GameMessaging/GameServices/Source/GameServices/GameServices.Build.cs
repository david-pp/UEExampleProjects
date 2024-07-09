// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameServices : ModuleRules
{
	public GameServices(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Messaging",
				"MessagingRpc",
				"MessagingCommon",
				"GameMessaging",
				"Json",
				"JsonUtilities", 
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Networking", 
			"GameTcpMessaging", 
			"GameNatsMessaging", 
		});

		PublicDefinitions.Add("WITH_GAME_SERVICES=1");
	}
}