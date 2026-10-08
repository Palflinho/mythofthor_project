using UnrealBuildTool;
using System.Collections.Generic;

public class MythOfThorEditorTarget : TargetRules
{
	public MythOfThorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MythOfThor");
	}
}
