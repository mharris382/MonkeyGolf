// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfTesting : ModuleRules
{
    public VRGolfTesting(ReadOnlyTargetRules Target) : base(Target)
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
                "UnrealEd",
                "PCG",
                "VRGolfEnviornment",
                "EnvironmentQueryEditor",
                "AIModule",
                "NavigationSystem",
                "GameplayTasks",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
