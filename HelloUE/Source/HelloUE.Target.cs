// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HelloUETarget : TargetRules
{
	public HelloUETarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new string[]
		{
			"HelloUE", 
			"CustomUMeshComponent", 
			"DeformMesh"
		});
	}
}
