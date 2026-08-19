// Copyright (c) Jared Taylor

#include "MobWorldActorFactory.h"

#include "MobWorldLightVolume.h"
#include "MobWorldSky.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldActorFactory)

#define LOCTEXT_NAMESPACE "MobWorldEditor"

UMobWorldSkyFactory::UMobWorldSkyFactory()
{
	DisplayName = LOCTEXT("SkyFactory", "Sky");
	NewActorClass = AMobWorldSky::StaticClass();
}

UMobWorldLightVolumeFactory::UMobWorldLightVolumeFactory()
{
	DisplayName = LOCTEXT("LightVolumeFactory", "Light Volume");
	NewActorClass = AMobWorldLightVolume::StaticClass();
}

#undef LOCTEXT_NAMESPACE
