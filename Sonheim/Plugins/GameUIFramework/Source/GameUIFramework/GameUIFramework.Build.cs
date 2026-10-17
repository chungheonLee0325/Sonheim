using UnrealBuildTool;

public class GameUIFramework : ModuleRules
{
	public GameUIFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UMG", "Slate", "SlateCore", "GameplayTags", "DeveloperSettings" });
	}
}
