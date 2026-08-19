// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "MobWorldActorFactory.generated.h"

/** Drops the backdrop. */
UCLASS()
class MOBWORLDEDITOR_API UMobWorldSkyFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UMobWorldSkyFactory();
};

/** Drops a light volume. */
UCLASS()
class MOBWORLDEDITOR_API UMobWorldLightVolumeFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UMobWorldLightVolumeFactory();
};
