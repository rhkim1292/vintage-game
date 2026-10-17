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
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LD60",
			"LD60/Variant_Platforming",
			"LD60/Variant_Platforming/Animation",
			"LD60/Variant_Combat",
			"LD60/Variant_Combat/AI",
			"LD60/Variant_Combat/Animation",
			"LD60/Variant_Combat/Gameplay",
			"LD60/Variant_Combat/Interfaces",
			"LD60/Variant_Combat/UI",
			"LD60/Variant_SideScrolling",
			"LD60/Variant_SideScrolling/AI",
			"LD60/Variant_SideScrolling/Gameplay",
			"LD60/Variant_SideScrolling/Interfaces",
			"LD60/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
