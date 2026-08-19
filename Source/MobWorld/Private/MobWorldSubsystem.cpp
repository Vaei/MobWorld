// Copyright (c) Jared Taylor

#include "MobWorldSubsystem.h"

#include "EngineUtils.h"
#include "MobFortStatics.h"
#include "MobWorldSettings.h"
#include "MobWorldSky.h"
#include "MobWorldSkySet.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldSubsystem)

namespace MobWorldWeatherParam
{
	/** On MobMaterials' collection, read by every surface master. */
	static const FName Wetness = TEXT("Wetness");
	static const FName Snow = TEXT("Snow");
	static const FName PuddleAmount = TEXT("PuddleAmount");
}

UMobWorldSubsystem* UMobWorldSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;

	return World ? World->GetSubsystem<UMobWorldSubsystem>() : nullptr;
}

void UMobWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SkyIndex = UMobWorldSettings::Get()->DefaultSkyIndex;
}

void UMobWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Here rather than in Initialize: the collection instances and the level's sky spheres do not
	// exist until the world is up, and a write before that lands on nothing.
	RequestSky();
}

const UMobWorldSkySet* UMobWorldSubsystem::GetSkySet() const
{
	return UMobWorldSettings::Get()->SkySet.LoadSynchronous();
}

FMobWorldSkyEntry UMobWorldSubsystem::GetSky() const
{
	const UMobWorldSkySet* Set = GetSkySet();
	const FMobWorldSkyEntry* Sky = Set ? Set->Find(SkyIndex) : nullptr;

	return Sky ? *Sky : FMobWorldSkyEntry();
}

UTextureCube* UMobWorldSubsystem::GetSkyCubemap() const
{
	return GetSky().Cubemap.Get();
}

bool UMobWorldSubsystem::IsSkyLoaded() const
{
	const FMobWorldSkyEntry Sky = GetSky();
	return Sky.Cubemap.IsNull() || Sky.Cubemap.Get() != nullptr;
}

void UMobWorldSubsystem::PreloadSky(const int32 Index)
{
	if (PreloadedIndex == Index && PreloadHandle.IsValid())
	{
		return;
	}

	if (PreloadHandle.IsValid())
	{
		PreloadHandle->ReleaseHandle();
		PreloadHandle.Reset();
	}

	PreloadedIndex = Index;

	const UMobWorldSkySet* Set = GetSkySet();
	const FMobWorldSkyEntry* Sky = Set ? Set->Find(Index) : nullptr;
	if (!Sky)
	{
		return;
	}

	TArray<FSoftObjectPath> Paths;
	Sky->GetAssetsToLoad(Paths);

	if (!Paths.IsEmpty())
	{
		PreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths);
	}
}

bool UMobWorldSubsystem::IsSkyPreloaded(const int32 Index) const
{
	return PreloadedIndex == Index && PreloadHandle.IsValid() && PreloadHandle->HasLoadCompleted();
}

void UMobWorldSubsystem::RequestSky()
{
	// The preload becomes the live handle rather than being dropped and asked for again, which is
	// what makes preloading worth doing: the assets are already in and the apply is immediate.
	if (PreloadedIndex == SkyIndex && PreloadHandle.IsValid())
	{
		if (SkyHandle.IsValid())
		{
			SkyHandle->ReleaseHandle();
		}

		SkyHandle = PreloadHandle;
		PreloadHandle.Reset();
		PreloadedIndex = INDEX_NONE;

		if (SkyHandle->HasLoadCompleted())
		{
			ApplySky();
		}
		else
		{
			SkyHandle->BindCompleteDelegate(
				FStreamableDelegate::CreateUObject(this, &UMobWorldSubsystem::OnSkyLoaded));
		}
		return;
	}

	// Released first, or walking through skies holds every one it passed.
	if (SkyHandle.IsValid())
	{
		SkyHandle->ReleaseHandle();
		SkyHandle.Reset();
	}

	TArray<FSoftObjectPath> Paths;
	GetSky().GetAssetsToLoad(Paths);

	if (Paths.IsEmpty())
	{
		ApplySky();
		return;
	}

	SkyHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		Paths, FStreamableDelegate::CreateUObject(this, &UMobWorldSubsystem::OnSkyLoaded));
}

void UMobWorldSubsystem::OnSkyLoaded()
{
	ApplySky();
}

float UMobWorldSubsystem::GetSpecular() const
{
	return SpecularOverride >= 0.f ? SpecularOverride : GetSky().Specular;
}

void UMobWorldSubsystem::SetSkyIndex(const int32 NewIndex)
{
	if (SkyIndex == NewIndex)
	{
		return;
	}

	SkyIndex = NewIndex;
	RequestSky();
}

void UMobWorldSubsystem::SetSpecularOverride(const float Specular)
{
	if (FMath::IsNearlyEqual(SpecularOverride, Specular))
	{
		return;
	}

	SpecularOverride = Specular;
	RefreshMaterials();
}

void UMobWorldSubsystem::SetWeather(const float Wetness, const float Snow, const float PuddleAmount)
{
	const UWorld* World = GetWorld();
	UMaterialParameterCollection* Collection =
		UMobWorldSettings::Get()->WeatherCollection.LoadSynchronous();

	if (!World || !Collection)
	{
		return;
	}

	if (UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection))
	{
		Instance->SetScalarParameterValue(MobWorldWeatherParam::Wetness, Wetness);
		Instance->SetScalarParameterValue(MobWorldWeatherParam::Snow, Snow);
		Instance->SetScalarParameterValue(MobWorldWeatherParam::PuddleAmount, PuddleAmount);
	}
}

void UMobWorldSubsystem::RefreshAllWorlds()
{
	if (!GEngine)
	{
		return;
	}

	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (UWorld* World = Context.World())
		{
			if (UMobWorldSubsystem* Subsystem = World->GetSubsystem<UMobWorldSubsystem>())
			{
				Subsystem->RefreshMaterials();
			}
		}
	}
}

void UMobWorldSubsystem::ApplySky()
{
	RefreshMaterials();
	RefreshSkyActors();

	OnSkyChanged.Broadcast(SkyIndex);
}

void UMobWorldSubsystem::ApplySkyToMaterial(UMaterialInstanceDynamic* Instance,
	const FGameplayTag GradientTag) const
{
	const FMobWorldSkyEntry Sky = GetSky();

	// Nulls and a negative scalar are left alone rather than cleared, so a sky that names no gradient
	// atlas leaves a character on the one its own material chose.
	UMobFortStatics::SetSky(Instance, Sky.Panorama.Get(), Sky.FindGradientAtlas(GradientTag).Get(),
		GetSpecular());

	if (Sky.MaxMip >= 0.f)
	{
		UMobFortStatics::SetMaxMip(Instance, Sky.MaxMip);
	}
}

void UMobWorldSubsystem::RefreshMaterials()
{
	for (int32 Index = TrackedInstances.Num() - 1; Index >= 0; --Index)
	{
		UMaterialInstanceDynamic* Instance = TrackedInstances[Index].Instance.Get();
		if (!Instance)
		{
			TrackedInstances.RemoveAtSwap(Index);
			continue;
		}

		ApplySkyToMaterial(Instance, TrackedInstances[Index].GradientTag);
	}
}

void UMobWorldSubsystem::RefreshSkyActors() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMobWorldSky> It(World); It; ++It)
	{
		It->RefreshSky();
	}
}

void UMobWorldSubsystem::RegisterComponent(UPrimitiveComponent* Component,
	TArray<UMaterialInstanceDynamic*>& OutInstances, const FGameplayTag GradientTag)
{
	UMobFortStatics::CreateDynamicMaterials(Component, OutInstances);
	RegisterMaterials(OutInstances, GradientTag);
}

void UMobWorldSubsystem::RegisterActor(AActor* Actor,
	TArray<UMaterialInstanceDynamic*>& OutInstances, const FName MeshTag,
	const FGameplayTag GradientTag)
{
	UMobFortStatics::CreateDynamicMaterialsForActor(Actor, OutInstances, MeshTag);
	RegisterMaterials(OutInstances, GradientTag);
}

void UMobWorldSubsystem::RegisterMaterials(const TArray<UMaterialInstanceDynamic*>& Instances,
	const FGameplayTag GradientTag)
{
	for (UMaterialInstanceDynamic* Instance : Instances)
	{
		if (!Instance)
		{
			continue;
		}

		ApplySkyToMaterial(Instance, GradientTag);

		FTrackedInstance* Existing = TrackedInstances.FindByPredicate(
			[Instance](const FTrackedInstance& Tracked) { return Tracked.Instance == Instance; });

		if (Existing)
		{
			Existing->GradientTag = GradientTag;
		}
		else
		{
			TrackedInstances.Add({ Instance, GradientTag });
		}
	}
}

void UMobWorldSubsystem::SetGradientTag(const TArray<UMaterialInstanceDynamic*>& Instances,
	const FGameplayTag GradientTag)
{
	RegisterMaterials(Instances, GradientTag);
}
