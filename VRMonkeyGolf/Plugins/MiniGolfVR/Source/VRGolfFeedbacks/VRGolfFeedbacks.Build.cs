// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfFeedbacks : ModuleRules
{
    public VRGolfFeedbacks(ReadOnlyTargetRules Target) : base(Target)
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
                "ISMRuntimeFeedbacks",
                "Niagara",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
