// Copyright 2020 Dan Kestranek.

using UnrealBuildTool;

public class GASDocumentation : ModuleRules
{
	public GASDocumentation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "GameplayAbilities" });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks", 
        });
        
        
        if (Target.Type == TargetType.Editor)
        {
	        PublicDependencyModuleNames.AddRange(
		        new string[] {
			        "UnrealEd",
			        "KismetCompiler",
			        "AssetTools",
			        "GameplayTagsEditor"
		        }
	        );
        }
    }
}
