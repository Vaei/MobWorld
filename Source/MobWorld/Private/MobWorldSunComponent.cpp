// Copyright (c) Jared Taylor

#include "MobWorldSunComponent.h"

#include "MobWorldSettings.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldSunComponent)

UMobWorldSunComponent::UMobWorldSunComponent()
{
	FortLighting = UMobWorldSettings::Get()->FortLighting;
}

void UMobWorldSunComponent::SyncFortLighting()
{
	if (FortLighting.IsNull())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// A preview world is a thumbnail or an asset editor, and its collection instance is not the one
	// anything is looking at.
	switch (World->WorldType)
	{
	case EWorldType::Game:
	case EWorldType::PIE:
	case EWorldType::Editor:
		break;
	default:
		return;
	}

	const UMaterialParameterCollection* Collection = FortLighting.LoadSynchronous();
	if (!Collection)
	{
		return;
	}

	UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection);
	if (!Instance)
	{
		return;
	}

	const FVector Direction = -GetDirection();
	Instance->SetVectorParameterValue(SunDirectionParameter, FLinearColor(
		static_cast<float>(Direction.X), static_cast<float>(Direction.Y),
		static_cast<float>(Direction.Z), 0.f));

	FLinearColor Colour = GetLightColor();
	FLinearColor Existing;
	Colour.A = Instance->GetVectorParameterValue(SunColorParameter, Existing) ? Existing.A : 1.f;
	Instance->SetVectorParameterValue(SunColorParameter, Colour);
}

void UMobWorldSunComponent::OnRegister()
{
	Super::OnRegister();

	SyncFortLighting();
}

void UMobWorldSunComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	// Colour and intensity reach the renderer by marking the render state dirty and nothing else,
	// so this is where a SetLightColor from Blueprint arrives.
	SyncFortLighting();
}

void UMobWorldSunComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();

	SyncFortLighting();
}
