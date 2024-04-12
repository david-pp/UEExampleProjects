// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class GameUserService : ModuleRules
	{
		public GameUserService(ReadOnlyTargetRules Target) : base(Target)
		{
            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                    "Messaging",
                    "MessagingRpc",
                }
            );

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
			);
			
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"GameServiceMessages", 
					"GameServiceRpc",
				}
			);

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                    "Messaging",
                    "MessagingRpc",
                    "GameServices",
                }
            );

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/GameService/Proxies/Private",
					"Runtime/GameService/Proxies/Private/User",
				}
			);
		}
	}
}
