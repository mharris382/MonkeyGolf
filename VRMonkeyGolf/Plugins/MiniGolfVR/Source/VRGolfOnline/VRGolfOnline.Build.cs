// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfOnline : ModuleRules
{
    public VRGolfOnline(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "VRGolf",
                "GameplayTags",
                "PhysicsCore",
                "OnlineSubsystem",
                "OnlineSubsystemEOS",
                "OnlineSubsystemUtils",
                "VRGolfSave",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
