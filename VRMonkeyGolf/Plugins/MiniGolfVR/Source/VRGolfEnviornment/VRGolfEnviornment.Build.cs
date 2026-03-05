// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfEnviornment : ModuleRules
{
    public VRGolfEnviornment(ReadOnlyTargetRules Target) : base(Target)
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
                "GeometryScriptingCore",
                "GeometryCore",
                "ModelingOperators",
                "PCGGeometryScriptInterop",
                "PCG",
                "PCGHelpers",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GeometryCore",
                "GeometryFramework",
                "ModelingComponents"
            }
        );
    }
}
