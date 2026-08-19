// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MobWorldSettings.generated.h"

class UMaterialParameterCollection;
class UMobWorldSkySet;

/**
 * Where MobWorld finds the things it joins together.
 *
 * All of it optional. A cleared collection is a plugin this project does not use, and MobWorld skips
 * it rather than complaining: that is what makes the set of Mob plugins a project installs its own
 * choice rather than this one's.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Mob World"))
class MOBWORLD_API UMobWorldSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMobWorldSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	static const UMobWorldSettings* Get() { return GetDefault<UMobWorldSettings>(); }

	/** Every sky the game can be under. Without one, nothing here has anything to apply. */
	UPROPERTY(Config, EditAnywhere, Category="Sky", meta=(AllowedClasses="/Script/MobWorld.MobWorldSkySet"))
	TSoftObjectPtr<UMobWorldSkySet> SkySet;

	/** Which sky a world starts under, before anything says otherwise. */
	UPROPERTY(Config, EditAnywhere, Category="Sky", meta=(ClampMin="0"))
	int32 DefaultSkyIndex = 0;

	/**
	 * MobFort's collection: the sun, the indirect response and the sky's yaw.
	 *
	 * Cleared, characters keep whatever the collection was last saved with and the sun stops
	 * following the light.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Collections")
	TSoftObjectPtr<UMaterialParameterCollection> FortLighting;

	/**
	 * MobMaterials' collection: how wet the world is, and how much snow is on it.
	 *
	 * Cleared, weather reaches characters but never the ground they are standing on.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Collections")
	TSoftObjectPtr<UMaterialParameterCollection> WeatherCollection;
};
