// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobWorldTypes.h"
#include "MobWorldSky.generated.h"

class UBillboardComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class USkyLightComponent;
class UStaticMeshComponent;
class UTextureCube;

/**
 * The sky, as something in the world rather than only something materials read.
 *
 * A C++ backdrop: the dome, the projection onto the ground, and the sky light that goes with it.
 * The same job the engine's HDRIBackdrop Blueprint does, in a class, so that what a character
 * reflects and what is behind them come from one place and cannot drift apart.
 *
 * Almost nothing is set on the actor. Which sky, how far round it is turned, how bright, how far
 * away - all of it lives on the entry in the sky set, because those are properties of the sky and
 * not of the level it happens to be standing in. Editing the set updates every backdrop that is
 * showing it, in the editor as you type.
 *
 * The yaw goes into MobFort's collection as well. Miss that and the sun is behind a character in
 * their reflection and in front of them on screen, which reads as a bug in the character.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Mob),
	HideCategories=(Input, Replication, Collision, HLOD, Physics, Networking, Cooking))
class MOBWORLD_API AMobWorldSky : public AActor
{
	GENERATED_BODY()

public:
	AMobWorldSky();

	/**
	 * Which sky to show, when this backdrop is not showing the one the world is standing under.
	 *
	 * Below zero follows the world, which is what lets a mission or a chapter change the sky without
	 * anybody touching the level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(ClampMin="-1"))
	int32 SkyIndexOverride = INDEX_NONE;

	/**
	 * Whether this backdrop's sky decides how the world's characters reflect.
	 *
	 * On, which is the point of it. Off for a second backdrop in a level, or one shown somewhere
	 * that should not be deciding the lighting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky")
	bool bDrivesMaterials = true;

	/**
	 * Where the sky is projected from, relative to this actor.
	 *
	 * The point the ground projection is computed around, which is usually where the camera spends
	 * its time rather than where the backdrop's origin happens to sit. Drag the widget in the
	 * viewport.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky", meta=(MakeEditWidget=true))
	FVector ProjectionCenter = FVector::ZeroVector;

	/** Whether a sky light is driven from the same cubemap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky")
	bool bCreateSkyLight = true;

	/**
	 * The material on the dome's sky half.
	 *
	 * The dome carries two slots, Sky and Floor, and they are different shaders. One material in
	 * both draws the ground projection across the sky.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky|Advanced", AdvancedDisplay)
	TSoftObjectPtr<UMaterialInterface> SkyMaterial;

	/** The material on the dome's ground half. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky|Advanced", AdvancedDisplay)
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	/** The dome, when the sky names none of its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky|Advanced", AdvancedDisplay)
	TSoftObjectPtr<UStaticMesh> DefaultMesh;

	/** The collection the yaw is written into, which is the one MobFort's masters read. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky|Advanced", AdvancedDisplay)
	TSoftObjectPtr<UMaterialParameterCollection> FortLighting;

	/** @return The sky being shown: the override, or the one the world is under. */
	UFUNCTION(BlueprintPure, Category="Sky")
	FMobWorldSkyEntry GetSky() const;

	/** @return Where the sky projects from, in world space. */
	UFUNCTION(BlueprintPure, Category="Sky")
	FVector GetProjectionCenter() const;

	/** @return How far round the sky is turned. */
	UFUNCTION(BlueprintPure, Category="Sky")
	float GetSkyYaw() const;

	/** Redraws the backdrop from the sky it is showing. Called for you when that sky changes. */
	UFUNCTION(BlueprintCallable, Category="Sky")
	void RefreshSky();

	UFUNCTION(BlueprintPure, Category="Sky")
	UStaticMeshComponent* GetSkyMesh() const { return SkyMesh; }

	UFUNCTION(BlueprintPure, Category="Sky")
	USkyLightComponent* GetSkyLight() const { return SkyLight; }

	/** Tells every backdrop in every live world to redraw. What the sky set calls when it is edited. */
	static void RefreshAll();

public:
	//~ Begin AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End AActor Interface

protected:
	/**
	 * Writes the yaw and the projection centre into the collection.
	 *
	 * Both go out so a material that is not the backdrop's can line up with it. A collection that
	 * declares neither parameter takes neither, so this costs an unconfigured project nothing.
	 */
	void WriteCollection(float Yaw) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UStaticMeshComponent> SkyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<USkyLightComponent> SkyLight;

	/** Applies the sky to one slot, making its instance dynamic first. */
	void ApplyToSlot(FName SlotName, const TSoftObjectPtr<UMaterialInterface>& Source,
		TObjectPtr<UMaterialInstanceDynamic>& Instance, const FMobWorldSkyEntry& Sky,
		UTextureCube* Cubemap);

	/** The billboard, so the backdrop can be clicked on rather than only found in the outliner. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sky")
	TObjectPtr<UBillboardComponent> Sprite;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SkyMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorMaterialInstance;
};
