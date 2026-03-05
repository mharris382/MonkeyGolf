// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfPlayer : ModuleRules
{
    public VRGolfPlayer(ReadOnlyTargetRules Target) : base(Target)
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
                "EnhancedInput",
                "XRBase",
                "VRExpansionPlugin",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
