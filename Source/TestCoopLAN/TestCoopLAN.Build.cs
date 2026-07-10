// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestCoopLAN : ModuleRules
{
    public TestCoopLAN(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",

			// Online Subsystem base
			"OnlineSubsystem",
            "OnlineSubsystemUtils"
        });


        PrivateDependencyModuleNames.AddRange(new string[]
        {
			// Steam online/session system
			"OnlineSubsystemSteam",

			// SteamSockets NetDriver
			"SteamSockets",

			// Steam socket subsystem support
			"SocketSubsystemSteamIP"
        });

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

    }
}