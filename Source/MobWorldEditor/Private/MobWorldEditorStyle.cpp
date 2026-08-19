// Copyright (c) Jared Taylor

#include "MobWorldEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FMobWorldEditorStyle::StyleSet;

FName FMobWorldEditorStyle::GetStyleSetName()
{
	static const FName StyleName(TEXT("MobWorldEditorStyle"));
	return StyleName;
}

void FMobWorldEditorStyle::Register()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("MobWorld"));
	if (!Plugin.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());
	StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));

	// Registered only when the file is really there. A brush pointing at a missing png logs an error
	// on every draw of the toolbar, which is a worse way to find out the art is absent than the
	// button simply having no icon.
	const FString IconPath = StyleSet->RootToContentDir(TEXT("Icon64"), TEXT(".png"));
	if (FPaths::FileExists(IconPath))
	{
		// 16 square is what a toolbar entry draws at; anything larger is downsampled every frame.
		StyleSet->Set(GetMenuIconName(), new FSlateImageBrush(IconPath, FVector2D(16.f, 16.f)));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FMobWorldEditorStyle::Unregister()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}
