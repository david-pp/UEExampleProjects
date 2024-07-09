// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class GameUserService : ModuleRules
	{
		public GameUserService(ReadOnlyTargetRules Target) : base(Target)
		{
			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Messaging",
					"MessagingRpc",
					"GameMessaging",
					"GameServices",
					"GameServiceMessages",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
				
				}
			);

			PrivateIncludePaths.AddRange(
				new string[]
				{
				}
			);
		}
	}
}