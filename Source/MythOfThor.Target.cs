using UnrealBuildTool;
using System.Collections.Generic;

public class MythOfThorTarget : TargetRules
{
	public MythOfThorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MythOfThor");
	}
}
