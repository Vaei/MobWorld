// Copyright (c) Jared Taylor

#include "MobWorldLightVolume.h"

#include "MobWorldTypes.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldLightVolume)

namespace MobWorldLightVolumeDefaults
{
	/** Drawn in the viewport, and never anywhere else. */
	static const FColor Wireframe(80, 80, 160);
}

AMobWorldLightVolume::AMobWorldLightVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
#if WITH_EDITORONLY_DATA
		MobWorldSprite::Apply(Sprite);
#endif
		Sprite->SetupAttachment(RootComponent);
		Sprite->bIsScreenSizeScaled = true;
		Sprite->SetHiddenInGame(true);
	}
}

bool AMobWorldLightVolume::ContainsPoint(const FVector& Point) const
{
	// Into the actor's own space, so a rotated or scaled volume is still the simple test and the room
	// can sit at whatever angle the building does.
	const FVector Local = GetActorTransform().InverseTransformPosition(Point);

	switch (Shape)
	{
	case EMobWorldLightShape::Sphere:
		return Local.SizeSquared() <= FMath::Square(Radius);

	case EMobWorldLightShape::Capsule:
		{
			// The nearest point on the capsule's spine, then a sphere test against that.
			const float Spine = FMath::Max(HalfHeight - Radius, 0.f);
			const float SpineZ = FMath::Clamp(Local.Z, -Spine, Spine);
			return FVector(Local.X, Local.Y, Local.Z - SpineZ).SizeSquared() <= FMath::Square(Radius);
		}

	case EMobWorldLightShape::Box:
	default:
		return FMath::Abs(Local.X) <= Extent.X
			&& FMath::Abs(Local.Y) <= Extent.Y
			&& FMath::Abs(Local.Z) <= Extent.Z;
	}
}

#if WITH_EDITOR
void AMobWorldLightVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	RebuildShape();
}

void AMobWorldLightVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Property = PropertyChangedEvent.GetMemberPropertyName();
	if (Property == GET_MEMBER_NAME_CHECKED(AMobWorldLightVolume, Shape))
	{
		RebuildShape();
	}
	else
	{
		ApplyShapeSize();
	}
}

void AMobWorldLightVolume::RebuildShape()
{
	ShapeComponent = nullptr;

	// Every shape, not just the one this actor knows about: a volume placed before the shape became a
	// drop-down saved one of each into the level, and they still load and draw.
	TInlineComponentArray<UShapeComponent*> Existing(this);
	for (UShapeComponent* Component : Existing)
	{
		Component->DestroyComponent();
	}

	UClass* Class = UBoxComponent::StaticClass();
	switch (Shape)
	{
	case EMobWorldLightShape::Sphere:	Class = USphereComponent::StaticClass();	break;
	case EMobWorldLightShape::Capsule:	Class = UCapsuleComponent::StaticClass();	break;
	default:																		break;
	}

	// Unnamed, because the one it replaces is destroyed but not yet collected and would clash.
	ShapeComponent = NewObject<UShapeComponent>(this, Class, NAME_None, RF_Transient);
	ShapeComponent->bIsEditorOnly = true;
	ShapeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShapeComponent->SetGenerateOverlapEvents(false);
	ShapeComponent->SetHiddenInGame(true);
	ShapeComponent->ShapeColor = MobWorldLightVolumeDefaults::Wireframe;
	ShapeComponent->SetupAttachment(RootComponent);
	ShapeComponent->RegisterComponent();

	ApplyShapeSize();
}

void AMobWorldLightVolume::ApplyShapeSize()
{
	if (UBoxComponent* AsBox = Cast<UBoxComponent>(ShapeComponent))
	{
		AsBox->SetBoxExtent(Extent);
	}
	else if (USphereComponent* AsSphere = Cast<USphereComponent>(ShapeComponent))
	{
		AsSphere->SetSphereRadius(Radius);
	}
	else if (UCapsuleComponent* AsCapsule = Cast<UCapsuleComponent>(ShapeComponent))
	{
		AsCapsule->SetCapsuleSize(Radius, HalfHeight);
	}
}
#endif
