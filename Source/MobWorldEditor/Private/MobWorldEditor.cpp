// Copyright (c) Jared Taylor

#include "MobWorldEditor.h"

#include "MobWorldEditorStyle.h"
#include "MobWorldEditorUserSettings.h"
#include "MobWorldSettings.h"
#include "MobWorldLightVolume.h"
#include "MobWorldSky.h"
#include "MobWorldSkySet.h"
#include "MobWorldActorFactory.h"
#include "SMobWorldMenuEntry.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "Factories/DataAssetFactory.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "IPythonScriptPlugin.h"
#include "ISettingsModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "LevelEditorViewport.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "ToolMenus.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "MobWorldEditor"

namespace MobWorldEditorDefaults
{
	/** Every Mob plugin keeps its toolbar toggle under this name, on a class named like this. */
	static const TCHAR* SettingsClassSuffix = TEXT("EditorUserSettings");
	static const TCHAR* SettingsClassPrefix = TEXT("Mob");
	static const TCHAR* ToolbarFlag = TEXT("bShowToolbarMenu");

	/** Project content, because which skies exist is the project's data and not the plugin's. */
	static const TCHAR* SkySetPackage = TEXT("/Game/MobWorld");
	static const TCHAR* SkySetName = TEXT("DA_SkySet");
}

void FMobWorldEditorModule::StartupModule()
{
	FMobWorldEditorStyle::Register();

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this, &FMobWorldEditorModule::RegisterMenus));
	}
}

void FMobWorldEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FMobWorldEditorStyle::Unregister();
}

void FMobWorldEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	if (!ToolBar)
	{
		return;
	}

	FToolMenuEntry Entry = FToolMenuEntry::InitComboButton(
		TEXT("WorldMenu"),
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction(),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateStatic(&FMobWorldEditorModule::IsToolbarMenuEnabled)),
		FOnGetContent::CreateRaw(this, &FMobWorldEditorModule::BuildMenu),
		LOCTEXT("WorldToolbar", "World"),
		LOCTEXT("WorldToolbarTip", "The sky, and what it is joined to"),
		FSlateIcon(FMobWorldEditorStyle::GetStyleSetName(), FMobWorldEditorStyle::GetMenuIconName()));

	// The style that gives a toolbar button its label beside the icon.
	Entry.StyleNameOverride = TEXT("CalloutToolbar");

	ToolBar->FindOrAddSection(TEXT("PlayGameExtensions")).AddEntry(Entry);
}

bool FMobWorldEditorModule::IsToolbarMenuEnabled()
{
	return UMobWorldEditorUserSettings::Get()->bShowToolbarMenu;
}

TSharedRef<SWidget> FMobWorldEditorModule::BuildMenu()
{
	FMenuBuilder Menu(true, nullptr);

	Menu.BeginSection(TEXT("WorldSetup"), LOCTEXT("SetupSection", "Setup"));
	Menu.AddMenuEntry(
		LOCTEXT("SetUp", "Set Up This Project"),
		LOCTEXT("SetUpTip",
			"Makes a sky set if there is none, points the Mob World settings at it, builds the "
			"material the backdrop draws with, and puts a backdrop in the open level.\n\n"
			"Everything it does is something you could do by hand; it exists so that installing the "
			"plugins and having a sky are the same step. Safe to run twice - nothing already set up "
			"is replaced."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Plus")),
		FUIAction(FExecuteAction::CreateStatic(&FMobWorldEditorModule::SetUpProject)));

	// Widgets rather than menu entries, because a menu entry only clicks: dragging one into the level
	// needs a widget that starts a drag the viewport understands.
	if (UActorFactory* Factory = GEditor
		? GEditor->FindActorFactoryByClass(UMobWorldSkyFactory::StaticClass()) : nullptr)
	{
		Menu.AddWidget(
			SNew(SMobWorldMenuEntry, Factory,
				FAppStyle::Get().GetBrush(TEXT("ClassIcon.SkyLight")),
				AMobWorldSky::StaticClass())
				.Label(LOCTEXT("AddSky", "Sky"))
				.ToolTip(LOCTEXT("AddSkyTip",
					"The whole backdrop: the dome, the projection onto the ground and the sky light.\n\n"
					"Which sky it shows and how far round that sky is turned live on the sky set. Where "
					"you put it is the projection centre.")),
			FText::GetEmpty(), true);
	}

	if (UActorFactory* Factory = GEditor
		? GEditor->FindActorFactoryByClass(UMobWorldLightVolumeFactory::StaticClass()) : nullptr)
	{
		Menu.AddWidget(
			SNew(SMobWorldMenuEntry, Factory,
				FAppStyle::Get().GetBrush(TEXT("ClassIcon.Volume")),
				AMobWorldLightVolume::StaticClass())
				.Label(LOCTEXT("AddLightVolume", "Light Volume"))
				.ToolTip(LOCTEXT("AddLightVolumeTip",
					"Darkens the characters standing inside it.\n\n"
					"An unlit character has no light to walk out of, so a cellar leaves them as bright "
					"as the courtyard they came from. This is how a room says otherwise, and it is per "
					"character: standing in the dark watching somebody in the light keeps them lit.\n\n"
					"Needs bAreaLighting ticked on the character's material.")),
			FText::GetEmpty(), true);
	}

	Menu.AddMenuEntry(
		LOCTEXT("OpenSkySet", "Edit Skies"),
		LOCTEXT("OpenSkySetTip",
			"Opens the sky set: every sky the game can be under, and everything that changes with "
			"one.\n\n"
			"Editing it updates the backdrop and every character material as you type, so turning a "
			"sky is something you watch rather than something you save and reload to see."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
		FUIAction(FExecuteAction::CreateStatic(&FMobWorldEditorModule::OpenSkySet)));

	Menu.AddMenuEntry(
		LOCTEXT("Panorama", "Panorama From Cubemap"),
		LOCTEXT("PanoramaTip",
			"Bakes the cubemaps selected in the content browser into the long/lat images an unlit "
			"character reflects, one beside each cube in a Panorama folder.\n\n"
			"A cubemap cannot be reflected by MobFort and an .hdr imports as one whatever the "
			"import settings say, which is what this is for. Put the result on the sky's Panorama, "
			"and the MaxMip the Output Log reports beside it."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("ClassIcon.TextureCube")),
		FUIAction(
			FExecuteAction::CreateStatic(&FMobWorldEditorModule::ConvertSelectedToPanorama),
			FCanExecuteAction::CreateStatic(&FMobWorldEditorModule::HasCubemapSelected)));
	Menu.EndSection();

	Menu.BeginSection(TEXT("WorldBuild"), LOCTEXT("BuildSection", "Build"));
	Menu.AddMenuEntry(
		LOCTEXT("Rebuild", "Rebuild Content"),
		LOCTEXT("RebuildTip",
			"Re-makes the material instance the backdrop draws with, over the engine HDRI Backdrop "
			"plugin's projection master.\n\n"
			"Only needed after that instance has been deleted or the engine's master has moved. Set "
			"Up This Project runs it for you."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Refresh")),
		FUIAction(
			FExecuteAction::CreateStatic(&FMobWorldEditorModule::RebuildContent),
			FCanExecuteAction::CreateStatic(&FMobWorldEditorModule::IsPythonAvailable)));
	Menu.EndSection();

	Menu.BeginSection(TEXT("WorldSettings"), LOCTEXT("SettingsSection", "Settings"));
	Menu.AddMenuEntry(
		LOCTEXT("Settings", "Project Settings"),
		LOCTEXT("SettingsTip", "Which sky set, and which collections the plugins are joined through."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Toolbar.Settings")),
		FUIAction(FExecuteAction::CreateStatic(&FMobWorldEditorModule::OpenSettings)));

	const bool bAnyVisible = AreAnyMobMenusVisible();
	Menu.AddMenuEntry(
		bAnyVisible ? LOCTEXT("HideAll", "Hide All Mob Menus") : LOCTEXT("ShowAll", "Show All Mob Menus"),
		LOCTEXT("HideAllTip",
			"Takes every Mob plugin's button off your toolbar, or puts them all back.\n\n"
			"Found by reflection rather than by MobWorld depending on each plugin, so it covers "
			"whichever of them this project has installed and nothing breaks when one is removed."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Visibility")),
		FUIAction(FExecuteAction::CreateStatic(&FMobWorldEditorModule::SetAllMobMenusVisible,
			!bAnyVisible)));

	Menu.AddMenuEntry(
		LOCTEXT("HideMenu", "Hide This Menu"),
		LOCTEXT("HideMenuTip",
			"Removes the World button from your toolbar. Turn it back on under Editor Preferences, "
			"Plugins, Mob World Editor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Visibility")),
		FUIAction(FExecuteAction::CreateStatic(&FMobWorldEditorModule::HideToolbarMenu)));
	Menu.EndSection();

	return Menu.MakeWidget();
}

void FMobWorldEditorModule::Notify(const FText& Message, const bool bSuccess)
{
	FNotificationInfo Info(Message);
	Info.ExpireDuration = 6.f;

	const TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
	if (Item.IsValid())
	{
		Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
}

UMobWorldSkySet* FMobWorldEditorModule::EnsureSkySet()
{
	UMobWorldSettings* Settings = GetMutableDefault<UMobWorldSettings>();

	if (UMobWorldSkySet* Existing = Settings->SkySet.LoadSynchronous())
	{
		return Existing;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(
		TEXT("AssetTools")).Get();

	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(
		FString(MobWorldEditorDefaults::SkySetPackage) / MobWorldEditorDefaults::SkySetName,
		FString(), PackageName, AssetName);

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UMobWorldSkySet::StaticClass();

	UMobWorldSkySet* SkySet = Cast<UMobWorldSkySet>(AssetTools.CreateAsset(
		AssetName, FPackageName::GetLongPackagePath(PackageName),
		UMobWorldSkySet::StaticClass(), Factory));

	if (!SkySet)
	{
		return nullptr;
	}

	// Written to the config as well as to the object, or the next editor start comes up with no set
	// and setup looks like it never ran.
	Settings->SkySet = SkySet;
	Settings->SaveConfig();

	return SkySet;
}

void FMobWorldEditorModule::SetUpProject()
{
	RebuildContent();

	UMobWorldSkySet* SkySet = EnsureSkySet();
	if (!SkySet)
	{
		Notify(LOCTEXT("SetUpNoSet", "World: could not make a sky set. See the Output Log."), false);
		return;
	}

	AddSkyToLevel();

	Notify(FText::Format(
		LOCTEXT("SetUpDone", "World: set up. Put a cubemap and its panorama into {0}."),
		FText::FromString(SkySet->GetName())));
}

bool FMobWorldEditorModule::LevelHasSky()
{
	const UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return false;
	}

	for (TActorIterator<AMobWorldSky> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return true;
	}

	return false;
}

void FMobWorldEditorModule::AddSkyToLevel()
{
	if (LevelHasSky())
	{
		Notify(LOCTEXT("SkyExists", "World: this level already has a sky."));
		return;
	}

	UEditorActorSubsystem* Actors = GEditor
		? GEditor->GetEditorSubsystem<UEditorActorSubsystem>()
		: nullptr;

	if (!Actors)
	{
		return;
	}

	// At the origin, because that is the projection centre and a level is usually built around it.
	AActor* Sky = Actors->SpawnActorFromClass(AMobWorldSky::StaticClass(), FVector::ZeroVector);
	if (Sky)
	{
		Sky->SetActorLabel(TEXT("Sky"));
		Notify(LOCTEXT("SkyAdded", "World: sky added to the level."));
	}
}

void FMobWorldEditorModule::OpenSkySet()
{
	UMobWorldSkySet* SkySet = EnsureSkySet();
	if (!SkySet)
	{
		Notify(LOCTEXT("NoSet", "World: could not make a sky set. See the Output Log."), false);
		return;
	}

	if (GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(SkySet);
	}
}

bool FMobWorldEditorModule::IsPythonAvailable()
{
	return IPythonScriptPlugin::Get() && IPythonScriptPlugin::Get()->IsPythonAvailable();
}

bool FMobWorldEditorModule::RunPython(const FString& Snippet, const FText& DoneMessage)
{
	if (!IsPythonAvailable())
	{
		return false;
	}

	// Both plugins' folders, because the panorama bake is MobFort's script.
	FString Command;
	for (const TCHAR* PluginName : { TEXT("MobWorld"), TEXT("MobFort") })
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
		if (!Plugin.IsValid())
		{
			continue;
		}

		const FString Dir = FPaths::Combine(
			FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()), TEXT("Python"))
			.Replace(TEXT("\\"), TEXT("/"));

		Command += FString::Printf(
			TEXT("import sys%sif r'%s' not in sys.path: sys.path.append(r'%s')%s"),
			LINE_TERMINATOR, *Dir, *Dir, LINE_TERMINATOR);
	}

	Command += Snippet;

	const bool bOk = IPythonScriptPlugin::Get()->ExecPythonCommand(*Command);
	if (!bOk)
	{
		Notify(LOCTEXT("PythonFailed", "World: failed. See the Output Log."), false);
	}
	else if (!DoneMessage.IsEmpty())
	{
		Notify(DoneMessage);
	}

	return bOk;
}

void FMobWorldEditorModule::RebuildContent()
{
	RunPython(
		TEXT("import importlib, author_world; importlib.reload(author_world); author_world.build_all()"),
		FText::GetEmpty());
}

void FMobWorldEditorModule::ConvertSelectedToPanorama()
{
	RunPython(
		TEXT("import importlib, fort_panorama; importlib.reload(fort_panorama); ")
		TEXT("fort_panorama.convert_selected()"),
		LOCTEXT("PanoramaDone", "World: panoramas baked. See the Output Log for their MaxMip."));
}

bool FMobWorldEditorModule::HasCubemapSelected()
{
	const FContentBrowserModule* ContentBrowser =
		FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));

	if (!ContentBrowser)
	{
		return false;
	}

	TArray<FAssetData> Selected;
	ContentBrowser->Get().GetSelectedAssets(Selected);

	// The class name rather than the asset: asking for the object would load every cube the browser
	// is showing, every time the menu is opened.
	for (const FAssetData& Asset : Selected)
	{
		if (Asset.AssetClassPath == UTextureCube::StaticClass()->GetClassPathName())
		{
			return true;
		}
	}

	return false;
}

void FMobWorldEditorModule::OpenSettings()
{
	if (ISettingsModule* Settings = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings")))
	{
		// The section identifier, which is what UDeveloperSettings registers under. The display name
		// beside it in the tree is a different string and does not open anything.
		Settings->ShowViewer(TEXT("Project"), UMobWorldSettings::Get()->GetCategoryName(),
			UMobWorldSettings::Get()->GetSectionName());
	}
}

void FMobWorldEditorModule::SetAllMobMenusVisible(const bool bVisible)
{
	int32 Changed = 0;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UDeveloperSettings::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		const FString Name = Class->GetName();
		if (!Name.StartsWith(MobWorldEditorDefaults::SettingsClassPrefix)
			|| !Name.EndsWith(MobWorldEditorDefaults::SettingsClassSuffix))
		{
			continue;
		}

		FBoolProperty* Flag = FindFProperty<FBoolProperty>(Class, MobWorldEditorDefaults::ToolbarFlag);
		if (!Flag)
		{
			continue;
		}

		UObject* Settings = Class->GetDefaultObject();
		if (Flag->GetPropertyValue_InContainer(Settings) == bVisible)
		{
			continue;
		}

		Flag->SetPropertyValue_InContainer(Settings, bVisible);
		Settings->SaveConfig();
		++Changed;
	}

	Notify(FText::Format(bVisible
		? LOCTEXT("MobMenusShown", "World: showed {0} Mob menu(s).")
		: LOCTEXT("MobMenusHidden", "World: hid {0} Mob menu(s)."), Changed));
}

bool FMobWorldEditorModule::AreAnyMobMenusVisible()
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UDeveloperSettings::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		const FString Name = Class->GetName();
		if (!Name.StartsWith(MobWorldEditorDefaults::SettingsClassPrefix)
			|| !Name.EndsWith(MobWorldEditorDefaults::SettingsClassSuffix))
		{
			continue;
		}

		if (const FBoolProperty* Flag =
			FindFProperty<FBoolProperty>(Class, MobWorldEditorDefaults::ToolbarFlag))
		{
			if (Flag->GetPropertyValue_InContainer(Class->GetDefaultObject()))
			{
				return true;
			}
		}
	}

	return false;
}

void FMobWorldEditorModule::HideToolbarMenu()
{
	UMobWorldEditorUserSettings* Settings = GetMutableDefault<UMobWorldEditorUserSettings>();
	Settings->bShowToolbarMenu = false;
	Settings->SaveConfig();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMobWorldEditorModule, MobWorldEditor)
