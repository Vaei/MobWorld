// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "MobFortWetnessComponent.h"
#include "MobWorldWetnessComponent.generated.h"

class UMobWaterInteractionComponent;

/**
 * MobFort's wetness, answered by MobWater.
 *
 * MobFort keeps a high water mark and writes it as custom primitive data, and cannot know what a
 * project's water is, so it leaves SampleWaterline doing nothing. MobWater already measures the
 * surface every tick for wading and swimming. This is the one line between them, and it is the
 * whole reason MobWorld exists.
 *
 * Add it to a character instead of MobFort's own component. Everything else - drying, the high
 * water mark, which meshes are written - is inherited unchanged.
 */
UCLASS(ClassGroup=(Mob), meta=(BlueprintSpawnableComponent))
class MOBWORLD_API UMobWorldWetnessComponent : public UMobFortWetnessComponent
{
	GENERATED_BODY()

public:
	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	//~ End UActorComponent Interface

	/** @return The water the owner is standing in, if any. */
	UFUNCTION(BlueprintPure, Category="Wetness")
	UMobWaterInteractionComponent* GetWaterInteraction() const { return WaterInteraction; }

	/**
	 * @return Whether the owner is in water that is actually giving a surface back.
	 *
	 * Both halves, because they disagree: the interaction component reports being in water while its
	 * info is invalid, and anything trusting the flag alone treats a character on dry land as still
	 * standing in the pool.
	 */
	UFUNCTION(BlueprintPure, Category="Wetness")
	bool IsInWater() const;

protected:
	virtual bool SampleWaterline_Implementation(float& OutWorldZ) const override;

	UPROPERTY(Transient)
	TObjectPtr<UMobWaterInteractionComponent> WaterInteraction = nullptr;
};
