// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobWorldLightVolume.generated.h"

class UBillboardComponent;
class UShapeComponent;

/** What shape a darkened part of a level is. */
UENUM(BlueprintType)
enum class EMobWorldLightShape : uint8
{
	/** A rectangle. Rooms, corridors, cellars, the inside of a building. */
	Box			UMETA(DisplayName = "Box"),

	/** A sphere. The pool of shadow under a tree, a lamp's reach inverted, a cave mouth. */
	Sphere		UMETA(DisplayName = "Sphere"),

	/** A capsule. Wells, stairwells, chimneys, anything tall and round. */
	Capsule		UMETA(DisplayName = "Capsule"),
};

/**
 * A part of the level that is darker than the level as a whole.
 *
 * MobFort's characters are unlit, so there is no light for one to walk out of. What they shade
 * against is one collection describing the whole world, which is right until a character steps into
 * a cellar and stays as bright as the courtyard they left.
 *
 * Writing the collection would fix that character and darken every other one with them, which in a
 * game where you stand in the dark and watch somebody in the light is precisely backwards. So this
 * is per character: the volume says how dark it is inside, and each character carries their own
 * answer as primitive data.
 *
 * Overlapping volumes take the darkest, not the last or the nearest, so a nook inside a cellar is
 * darker than the cellar without anybody ordering them.
 */
UCLASS(Blueprintable, ClassGroup=(Mob), HideCategories=(Input, Replication, Networking, Physics, HLOD))
class MOBWORLD_API AMobWorldLightVolume : public AActor
{
	GENERATED_BODY()

public:
	AMobWorldLightVolume();

	/**
	 * How much darker a character inside is, 0 to 1.
	 *
	 * One is as dark as the collection's clamp allows rather than black: MobFort holds both its
	 * responses inside a window so a dark area cannot swallow a character entirely, which is the
	 * whole reason that window exists.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="0.0", ClampMax="1.0"))
	float Darken = 0.5f;

	/**
	 * How long a character takes to adjust on the way in and out, in seconds.
	 *
	 * Not instant. A character who changes brightness on a threshold reads as a material swapping
	 * rather than as a room getting darker.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="0.0", ForceUnits="s"))
	float BlendTime = 0.4f;

	/** What shape the darkened part of the level is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Light")
	EMobWorldLightShape Shape = EMobWorldLightShape::Box;

	/** How big it is, for a box. Scale the actor as well, or instead; both count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light",
		meta=(EditCondition="Shape == EMobWorldLightShape::Box", EditConditionHides))
	FVector Extent = FVector(400.f, 400.f, 200.f);

	/** How far it reaches, for a sphere or a capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="1.0",
		EditCondition="Shape != EMobWorldLightShape::Box", EditConditionHides))
	float Radius = 400.f;

	/** How tall it is, for a capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Light", meta=(ClampMin="1.0",
		EditCondition="Shape == EMobWorldLightShape::Capsule", EditConditionHides))
	float HalfHeight = 400.f;

	/** @return Whether a world point is inside this volume. */
	UFUNCTION(BlueprintPure, Category="Light")
	bool ContainsPoint(const FVector& Point) const;

protected:
#if WITH_EDITOR
	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Throws away whatever shape is there and makes the one the drop-down asks for. */
	void RebuildShape();

	/** Sizes the shape from whichever of the properties above belongs to it. */
	void ApplyShapeSize();
#endif

	/**
	 * The billboard, so the volume can be clicked on.
	 *
	 * A wireframe is only selectable by its edges, which in a level built of rooms means hunting for
	 * a line among the walls it is drawn against.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Light")
	TObjectPtr<UBillboardComponent> Sprite;

#if WITH_EDITORONLY_DATA
	/**
	 * The wireframe, and nothing else: containment is answered from the properties, not from here.
	 *
	 * Transient, and remade on load and on any change of shape, so what the level stores is the
	 * drop-down rather than a component that has to be kept agreeing with it.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UShapeComponent> ShapeComponent;
#endif
};
