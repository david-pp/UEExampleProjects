// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class GameServiceRpc : ModuleRules
	{
		public GameServiceRpc(ReadOnlyTargetRules Target) : base(Target)
		{
			DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                }
            );

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "MessagingRpc", 
                    "Messaging",
				}
			);

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                    "Messaging",
                    "MessagingCommon",
                    "MessagingRpc",
                }
            );

			PrivateIncludePaths.AddRange(
				new string[] {
				}
			);
		}
	}
}
