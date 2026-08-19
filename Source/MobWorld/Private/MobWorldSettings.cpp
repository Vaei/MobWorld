// Copyright (c) Jared Taylor

#include "MobWorldSettings.h"

#include "Materials/MaterialParameterCollection.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldSettings)

UMobWorldSettings::UMobWorldSettings()
{
	// Pointed at the plugins' own collections, so a project that installs MobFort and MobMaterials
	// and does nothing else still has the joins made.
	FortLighting = TSoftObjectPtr<UMaterialParameterCollection>(
		FSoftObjectPath(TEXT("/MobFort/MPC_FortLighting.MPC_FortLighting")));
	WeatherCollection = TSoftObjectPtr<UMaterialParameterCollection>(
		FSoftObjectPath(TEXT("/MobMaterials/MPC_MobWeather.MPC_MobWeather")));
}
