// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MobWorldEditorUserSettings.generated.h"

/**
 * Per-developer settings for this plugin. Not checked in.
 *
 * Editor preferences rather than project settings, because whether somebody wants the button on
 * their toolbar is theirs and not the project's.
 */
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="Mob World Editor"))
class UMobWorldEditorUserSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UMobWorldEditorUserSettings* Get() { return GetDefault<UMobWorldEditorUserSettings>(); }

	/** Whether the World button appears on the level editor toolbar. */
	UPROPERTY(Config, EditAnywhere, Category="Toolbar")
	bool bShowToolbarMenu = true;
};
