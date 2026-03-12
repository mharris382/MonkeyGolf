// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CourseGen : ModuleRules
{
	public CourseGen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "CoreUObject",
                "Engine",
                "PCG",
                "GeometryScriptingCore",
                "PCGGeometryScriptInterop",
                "ModelingOperators",
                "RHI",
                "RenderCore"
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
                    "UnrealEd"
                }
            );
        }
	}
}
