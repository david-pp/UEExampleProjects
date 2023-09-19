using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ThirdPersonMPEditor : ModuleRules
	{
		public ThirdPersonMPEditor(ReadOnlyTargetRules Target) : base(Target)
		{
            PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"PropertyEditor",
				"EditorStyle",
				"ThirdPersonMP",
			});

		PublicIncludePaths.AddRange(
			new string[]
			{
				"ThirdPersonMPEditor/Public"
			});

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"ThirdPersonMPEditor/Private"
			});
		}
	}
}
