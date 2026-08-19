// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;

/** The toolbar button's icon, read from the plugin's Resources folder. */
class FMobWorldEditorStyle
{
public:
	static void Register();
	static void Unregister();

	static FName GetStyleSetName();

	static FName GetMenuIconName()
	{
		static const FName IconName(TEXT("MobWorld.MenuIcon"));
		return IconName;
	}

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
