// Copyright (c) Jared Taylor

#include "MobWorldWetnessComponent.h"

#include "MobWaterInteractionComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldWetnessComponent)

void UMobWorldWetnessComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		WaterInteraction = Owner->FindComponentByClass<UMobWaterInteractionComponent>();
	}

	if (WaterInteraction)
	{
		// After the water, or the line is always one frame behind where the character is standing.
		AddTickPrerequisiteComponent(WaterInteraction);
	}
}

bool UMobWorldWetnessComponent::IsInWater() const
{
	return WaterInteraction && WaterInteraction->IsInWater()
		&& WaterInteraction->GetWaterInfo().bValid;
}

bool UMobWorldWetnessComponent::SampleWaterline_Implementation(float& OutWorldZ) const
{
	if (!IsInWater())
	{
		return false;
	}

	OutWorldZ = WaterInteraction->GetWaterInfo().SurfaceZ;
	return true;
}
