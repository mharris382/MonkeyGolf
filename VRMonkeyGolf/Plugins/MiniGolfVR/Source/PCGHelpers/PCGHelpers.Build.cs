// Copyright Max Harris

using UnrealBuildTool;

public class PCGHelpers : ModuleRules
{
    public PCGHelpers(ReadOnlyTargetRules Target) : base(Target)
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
                "PCG",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                
            }
        );
    }
}
