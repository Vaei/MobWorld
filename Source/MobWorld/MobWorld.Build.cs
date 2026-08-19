// Copyright (c) Jared Taylor

using UnrealBuildTool;

public class MobWorld : ModuleRules
{
	public MobWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"GameplayTags",
				"MobFort",
				"MobWater",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
			}
			);
	}
}
