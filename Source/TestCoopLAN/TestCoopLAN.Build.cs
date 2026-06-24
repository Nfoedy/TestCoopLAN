// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestCoopLAN : ModuleRules
{
	public TestCoopLAN(ReadOnlyTargetRules Target) : base(Target)
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
			"TestCoopLAN",
			"TestCoopLAN/Variant_Platforming",
			"TestCoopLAN/Variant_Platforming/Animation",
			"TestCoopLAN/Variant_Combat",
			"TestCoopLAN/Variant_Combat/AI",
			"TestCoopLAN/Variant_Combat/Animation",
			"TestCoopLAN/Variant_Combat/Gameplay",
			"TestCoopLAN/Variant_Combat/Interfaces",
			"TestCoopLAN/Variant_Combat/UI",
			"TestCoopLAN/Variant_SideScrolling",
			"TestCoopLAN/Variant_SideScrolling/AI",
			"TestCoopLAN/Variant_SideScrolling/Gameplay",
			"TestCoopLAN/Variant_SideScrolling/Interfaces",
			"TestCoopLAN/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
