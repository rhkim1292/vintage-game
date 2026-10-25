// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LD60 : ModuleRules
{
	public LD60(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"InputCore",
			"UMG",
			"SlateCore",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LD60",
			"LD60/Flipper"
		});
	}
}
