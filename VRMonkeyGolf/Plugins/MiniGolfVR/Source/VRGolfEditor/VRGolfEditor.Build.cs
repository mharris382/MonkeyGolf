// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfEditor : ModuleRules
{
    public VRGolfEditor(ReadOnlyTargetRules Target) : base(Target)
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
                "GeometryScriptingCore",
                "GameplayTags",
                "PhysicsCore",
                "PCG"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {

            }
        );
    }
}
