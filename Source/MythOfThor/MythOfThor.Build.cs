using UnrealBuildTool;

public class MythOfThor : ModuleRules
{
	public MythOfThor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"Paper2D",
			"UMG",
			"Synthesis",
			"MeshDescription",
			"StaticMeshDescription",
			"Slate",
			"SlateCore",
			"NavigationSystem",
			"Sockets",
			"Networking",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
