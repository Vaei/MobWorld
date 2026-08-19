// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SWidget;
class UMobWorldSkySet;

/**
 * The World button on the level editor toolbar.
 *
 * Everything behind it is setup rather than authoring: MobWorld draws almost nothing of its own, so
 * what a developer needs from it is a sky set, a backdrop in the level, the panoramas MobFort
 * reflects, and a shortcut to the settings that join the plugins together.
 *
 * Setup is C++ rather than Python on purpose. A plugin class only gets a Python binding once the
 * stub generator has seen it, so a script touching a class added since the editor started fails
 * with a missing attribute or a missing property - which is exactly the moment somebody is running
 * setup for the first time.
 */
class FMobWorldEditorModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface

protected:
	void RegisterMenus();
	TSharedRef<SWidget> BuildMenu();

	/**
	 * Makes a sky set, points the settings at it, builds the material and puts a backdrop in the level.
	 *
	 * Everything it does is something you could do by hand; it exists so that installing the plugins
	 * and having a sky are the same step. Safe to run twice - nothing already set up is replaced.
	 */
	static void SetUpProject();

	/** Puts a backdrop in the open level, if it has none. */
	static void AddSkyToLevel();

	/** @return Whether the open level already has a backdrop. */
	static bool LevelHasSky();

	/** Opens the sky set the settings name, making one first if there is none. */
	static void OpenSkySet();

	/** Bakes the selected cubemaps into panoramas, which is MobFort's script doing the work. */
	static void ConvertSelectedToPanorama();

	/** @return Whether the content browser selection holds a cubemap. */
	static bool HasCubemapSelected();

	/** Re-makes the material instance the backdrop draws with. */
	static void RebuildContent();

	static void OpenSettings();

	/** Takes the button off this developer's toolbar. */
	static void HideToolbarMenu();

	/**
	 * Shows or hides every Mob plugin's toolbar button at once.
	 *
	 * Found by reflection rather than by depending on each plugin: they all keep the same flag on a
	 * settings class named the same way, and MobWorld linking against every one of them to flip a
	 * bool would undo the point of the dependencies being droppable.
	 */
	static void SetAllMobMenusVisible(bool bVisible);

	/** @return Whether any Mob toolbar button is currently showing. */
	static bool AreAnyMobMenusVisible();

	/** @return Whether this developer wants the button at all. */
	static bool IsToolbarMenuEnabled();

	/** @return The set the settings name, making and saving one if they name none. */
	static UMobWorldSkySet* EnsureSkySet();

	/** @return Whether the Python plugin is available, which only the panorama bake needs. */
	static bool IsPythonAvailable();

	/** Runs a snippet with both plugins' Python folders on the path. */
	static bool RunPython(const FString& Snippet, const FText& DoneMessage);

	/** Tells the developer what happened, in the corner. */
	static void Notify(const FText& Message, bool bSuccess = true);
};
