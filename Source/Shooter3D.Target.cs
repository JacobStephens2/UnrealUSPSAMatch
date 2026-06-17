using UnrealBuildTool;
using System.Collections.Generic;

public class Shooter3DTarget : TargetRules
{
	public Shooter3DTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Shooter3D");
	}
}
