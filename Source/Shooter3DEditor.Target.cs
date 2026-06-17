using UnrealBuildTool;
using System.Collections.Generic;

public class Shooter3DEditorTarget : TargetRules
{
	public Shooter3DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Shooter3D");
	}
}
