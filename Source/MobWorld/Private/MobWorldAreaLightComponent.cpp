// Copyright (c) Jared Taylor

#include "MobWorldAreaLightComponent.h"

#include "MobFortTypes.h"
#include "MobWorldLightVolume.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldAreaLightComponent)

UMobWorldAreaLightComponent::UMobWorldAreaLightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMobWorldAreaLightComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshMeshes();
}

void UMobWorldAreaLightComponent::RefreshMeshes()
{
	Meshes.Reset();

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TInlineComponentArray<UMeshComponent*> Found(Owner);
	for (UMeshComponent* Mesh : Found)
	{
		if (MeshTags.IsEmpty() || MeshTags.ContainsByPredicate(
			[Mesh](const FName& Tag) { return Mesh->ComponentHasTag(Tag); }))
		{
			Meshes.Add(Mesh);
		}
	}

	Written = MAX_flt;
}

float UMobWorldAreaLightComponent::SampleVolumes(float& OutBlendTime) const
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return 0.f;
	}

	const FVector Point = Owner->GetActorLocation();
	float Darkest = 0.f;
	OutBlendTime = 0.4f;

	// The darkest rather than the first or the nearest, so a nook inside a cellar is darker than the
	// cellar without anybody having to order them.
	for (TActorIterator<AMobWorldLightVolume> It(World); It; ++It)
	{
		if (It->Darken > Darkest && It->ContainsPoint(Point))
		{
			Darkest = It->Darken;
			OutBlendTime = It->BlendTime;
		}
	}

	return Darkest;
}

void UMobWorldAreaLightComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceCheck += DeltaTime;
	if (TimeSinceCheck >= CheckInterval)
	{
		TimeSinceCheck = 0.f;

		float NewBlend = BlendTime;
		const float NewTarget = SampleVolumes(NewBlend);

		// The blend time comes from whichever volume is being entered, so a room decides how long it
		// takes to adjust to it. Leaving one keeps the time of the room being left behind.
		if (!FMath::IsNearlyEqual(NewTarget, Target))
		{
			Target = NewTarget;
			BlendTime = NewBlend;
		}
	}

	if (FMath::IsNearlyEqual(Current, Target))
	{
		return;
	}

	Current = BlendTime > 0.f
		? FMath::FInterpConstantTo(Current, Target, DeltaTime, 1.f / BlendTime)
		: Target;

	WriteMeshData();
}

void UMobWorldAreaLightComponent::WriteMeshData()
{
	if (FMath::IsNearlyEqual(Current, Written))
	{
		return;
	}

	Written = Current;

	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh)
		{
			Mesh->SetCustomPrimitiveDataFloat(MobFortData::LightDarken, Current);
		}
	}
}
