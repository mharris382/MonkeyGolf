// Copyright Max Harris

using UnrealBuildTool;

public class VRGolfUI : ModuleRules
{
    public VRGolfUI(ReadOnlyTargetRules Target) : base(Target)
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
                "Slate",
                "SlateCore",   // ← add
                "UMG",         // ← add (UWidgetComponent, UUserWidget)
                "InputCore",   // ← add (EKeys, for interactor later)
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VRGolfOnline",
                "VRGolfSave"
            }
        );
    }
}
