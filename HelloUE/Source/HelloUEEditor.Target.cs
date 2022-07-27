// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HelloUEEditorTarget : TargetRules
{
	public HelloUEEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new string[]
		{
			"HelloUE", 
			"CustomUMeshComponent", 
			"DeformMesh"
		});
	}
}
