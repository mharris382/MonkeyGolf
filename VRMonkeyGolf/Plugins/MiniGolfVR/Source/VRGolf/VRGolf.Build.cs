// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VRGolf : ModuleRules
{
	public VRGolf(ReadOnlyTargetRules Target) : base(Target)
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
				"InputCore",
                "GameplayTags",
                "EnhancedInput",
				"HeadMountedDisplay",
				"XRBase",
				"UMG",
				"SlateCore",
				"DeveloperSettings",
				"PhysicsCore"
                
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
