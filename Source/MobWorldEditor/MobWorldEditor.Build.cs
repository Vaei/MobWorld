// Copyright (c) Jared Taylor

using UnrealBuildTool;

public class MobWorldEditor : ModuleRules
{
	public MobWorldEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"DeveloperSettings",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"LevelEditor",
				"InputCore",
				"DeveloperSettings",
				"ToolMenus",
				"Projects",
				"Settings",
				"AssetTools",
				"AssetRegistry",
				"ContentBrowser",
				"PythonScriptPlugin",
				"MobWorld",
				"MobFort",
			}
			);
	}
}
