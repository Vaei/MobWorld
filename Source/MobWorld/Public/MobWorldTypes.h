// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MobWorldTypes.generated.h"

class UBillboardComponent;
class UStaticMesh;
class UTextureCube;
class UTexture2D;

#if WITH_EDITORONLY_DATA
namespace MobWorldSprite
{
	/**
	 * Puts the MobWorld icon on an actor's billboard.
	 *
	 * Call it from the actor's constructor. The finder underneath may only run while a class default
	 * object is being built, and every MobWorld actor wants the same icon.
	 */
	MOBWORLD_API void Apply(UBillboardComponent* Sprite);
}
#endif

/**
 * One sky, and everything that has to change with it.
 *
 * A struct rather than a set of parallel lists because these are one answer. Swapping the cube and
 * leaving the panorama or the gradients behind is a character lit for a different time of day, and
 * nothing in the engine notices: two lists of different lengths render, they just render wrong.
 *
 * Everything a sky owns is here, including how far round it is turned and how bright it is drawn.
 * The backdrop reads it rather than keeping its own copy, so a sky looks the same in every level it
 * appears in and there is one place to change it.
 *
 * The assets are soft. A set holds every sky a game has, and hard pointers would mean opening it
 * pulled every HDRI, every panorama and every atlas into memory at once - which is most of what a
 * stylised game's texture budget is. The subsystem loads the one sky being shown.
 */
USTRUCT(BlueprintType)
struct MOBWORLD_API FMobWorldSkyEntry
{
	GENERATED_BODY()

	/** What the sky shows, and what a lit material samples. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky")
	TSoftObjectPtr<UTextureCube> Cubemap;

	/**
	 * The same sky as a long/lat image, which is what an unlit MobFort character reflects.
	 *
	 * Made from the cube by Fort -> Panorama From Cubemap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky")
	TSoftObjectPtr<UTexture2D> Panorama;

	/**
	 * How far round the sky is turned, in degrees.
	 *
	 * Turns the backdrop and the panorama together. They are the same image, and a character
	 * reflecting a sun the backdrop put somewhere else is the failure this plugin exists to prevent,
	 * so there is one number and not two.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(ClampMin="-360.0", ClampMax="360.0", Units="Degrees"))
	float Yaw = 0.f;

	/** How bright the backdrop is drawn, and how strongly its sky light lights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(ClampMin="0.0"))
	float Intensity = 1.f;

	/**
	 * How this sky lights a character, as MobFort gradient atlases keyed by who is being lit.
	 *
	 * The whole atlas at once rather than a row at a time: a night sky moves the diffuse falloff,
	 * both skin ramps and both rims together, and picking that apart into five row indices is
	 * bookkeeping with nothing to gain.
	 *
	 * Keyed by tag rather than by character class or mesh, because what decides a character's
	 * gradients differs per project: a role for one, an outfit out of an inventory for another. A
	 * project that wants none of that leaves the one empty-tag entry alone and never passes a tag.
	 *
	 * The empty tag is the fallback for anything asking with a tag this sky does not list, so adding
	 * a sky never has to mean listing every character again.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky")
	TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> GradientAtlases;

	/**
	 * How hard this sky reflects off a character.
	 *
	 * The sky's own answer, because a moonlit sky reflects differently from a noon one whatever is
	 * happening under it. Below zero leaves each material on its own.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(ClampMin="-1.0"))
	float Specular = 0.12f;

	/**
	 * The mip a roughness of 1 reads, which is how long the panorama's chain is.
	 *
	 * Reported by Fort -> Panorama From Cubemap when it bakes one. Below zero leaves whatever the
	 * material says, which is right until the panorama's size stops matching it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(ClampMin="-1.0"))
	float MaxMip = -1.f;

	//~ The backdrop ----------------------------------------------------------------------------

	/**
	 * How far away the backdrop is drawn, in metres.
	 *
	 * Big enough to sit outside the level and no bigger. The ground projection is computed against
	 * this, so a sky the size of the solar system flattens it away.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop", meta=(ClampMin="1.0", Units="Meters"))
	float Size = 100.f;

	/**
	 * How far away the sky light treats the sky as being, as a multiple of Size.
	 *
	 * What decides whether the backdrop is scenery being lit or the sky doing the lighting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop", meta=(ClampMin="0.0"))
	float LightingDistanceFactor = 0.5f;

	/**
	 * Whether the image follows the camera instead of staying put against the ground.
	 *
	 * On, the backdrop stops tracking the ground, which is what a sky with no visible floor wants.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop")
	bool bUseCameraProjection = false;

	/** Whether the half below the horizon is drawn flat rather than as the image. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop")
	bool bLowerHemisphereIsBlack = false;

	/** What that half is drawn as, for a sky whose lower image is not worth showing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop", meta=(EditCondition="bLowerHemisphereIsBlack"))
	FLinearColor LowerHemisphereColor = FLinearColor::Black;

	/** The dome the sky is drawn on. Cleared, the backdrop keeps whichever mesh it already had. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backdrop", AdvancedDisplay)
	TSoftObjectPtr<UStaticMesh> Mesh;

	FMobWorldSkyEntry()
	{
		// One entry, under no tag, so a project that never thinks about this has a slot to fill and
		// a project that does has somewhere to add to.
		GradientAtlases.Add(FGameplayTag(), nullptr);
	}

	/**
	 * @return The atlas for whoever is being lit: theirs, or the untagged one, or null.
	 *
	 * Null is an answer, not a failure. It leaves a character on the atlas its own material names,
	 * which is what a project not using this wants.
	 */
	TSoftObjectPtr<UTexture2D> FindGradientAtlas(const FGameplayTag Tag) const
	{
		if (const TSoftObjectPtr<UTexture2D>* Exact = GradientAtlases.Find(Tag))
		{
			return *Exact;
		}

		const TSoftObjectPtr<UTexture2D>* Fallback = GradientAtlases.Find(FGameplayTag());
		return Fallback ? *Fallback : TSoftObjectPtr<UTexture2D>();
	}

	/** Everything this sky needs in memory before it can be shown. */
	void GetAssetsToLoad(TArray<FSoftObjectPath>& OutPaths) const
	{
		for (const TSoftObjectPtr<UTextureCube>& Ptr : { Cubemap })
		{
			if (!Ptr.IsNull()) { OutPaths.Add(Ptr.ToSoftObjectPath()); }
		}

		if (!Panorama.IsNull()) { OutPaths.Add(Panorama.ToSoftObjectPath()); }
		if (!Mesh.IsNull()) { OutPaths.Add(Mesh.ToSoftObjectPath()); }

		for (const TPair<FGameplayTag, TSoftObjectPtr<UTexture2D>>& Pair : GradientAtlases)
		{
			if (!Pair.Value.IsNull()) { OutPaths.Add(Pair.Value.ToSoftObjectPath()); }
		}
	}

	/** Whether this entry names a sky at all, as opposed to being an empty row somebody left behind. */
	bool IsValid() const { return !Cubemap.IsNull() || !Panorama.IsNull(); }
};
