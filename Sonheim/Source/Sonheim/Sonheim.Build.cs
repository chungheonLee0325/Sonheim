// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sonheim : ModuleRules
{
	public Sonheim(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "Niagara", "AIModule" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });
	}
}