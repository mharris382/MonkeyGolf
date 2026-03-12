// Copyright Max Harris

using UnrealBuildTool;

public class CourseGenCore : ModuleRules
{
    public CourseGenCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "CourseGen",
                "PCG",
                "GeometryScriptingCore",
                "PCGGeometryScriptInterop",
                "ModelingOperators",
                "RHI",
                "RenderCore",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "GeometryCore",
                "GeometryFramework",
                "ModelingComponents",
            }
        );

        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "AdvancedPreviewScene",
                    "UnrealEd",
                    "PCGEditor"
                }
            );
        }
    }
}
