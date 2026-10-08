using UnrealBuildTool;
using System.Collections.Generic;

public class MythOfThorEditorTarget : TargetRules
{
	public MythOfThorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MythOfThor");
	}
}
