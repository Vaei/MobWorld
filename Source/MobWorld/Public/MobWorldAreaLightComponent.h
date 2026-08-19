// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MobWorldAreaLightComponent.generated.h"

class UMeshComponent;

/**
 * How dark the room this actor is standing in is, written where the shading reads it.
 *
 * The one thing a level knows and an unlit master cannot: a character in a cellar and a character
 * in the courtyard shade against the same collection, and only something walking the world can tell
 * them apart. This does the walking and writes the answer as custom primitive data, so the two are
 * lit differently while still being one material.
 *
 * Costs one overlap test against the level's light volumes, on an interval rather than per frame.
 * A character standing still in a room that is not changing writes nothing at all.
 */
UCLASS(ClassGroup=(Mob), meta=(BlueprintSpawnableComponent))
class MOBWORLD_API UMobWorldAreaLightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMobWorldAreaLightComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent Interface

	/**
	 * Which meshes are written. Empty writes every mesh on the owner.
	 *
	 * The same set that gets the sky and the wetness, usually. A material with the feature switched
	 * off ignores what it is given, so writing one costs a render state update and nothing else.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light")
	TArray<FName> MeshTags;

	/** How often the level is asked which volume the owner is in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="0.02", ForceUnits="s"))
	float CheckInterval = 0.2f;

	/** @return How dark the owner's surroundings are right now, 0 to 1. */
	UFUNCTION(BlueprintPure, Category="Light")
	float GetDarken() const { return Current; }

	/** Finds the meshes to write again. Call after adding or swapping one at run time. */
	UFUNCTION(BlueprintCallable, Category="Light")
	void RefreshMeshes();

protected:
	/** @return The darkest volume the owner is standing in, or zero for none. */
	float SampleVolumes(float& OutBlendTime) const;

	/** Writes the current value to every target mesh. */
	void WriteMeshData();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> Meshes;

	/** Where the blend is now, and where it is heading. */
	float Current = 0.f;
	float Target = 0.f;

	/** Taken from whichever volume set the target, so each room decides its own adjustment. */
	float BlendTime = 0.4f;

	float TimeSinceCheck = 0.f;

	/** What was last written, so a character standing still costs no render state updates. */
	float Written = MAX_flt;
};
