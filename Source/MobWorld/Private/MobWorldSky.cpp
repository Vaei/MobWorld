// Copyright (c) Jared Taylor

#include "MobWorldSky.h"

#include "MobWorldSettings.h"
#include "MobWorldSkySet.h"
#include "MobWorldSubsystem.h"

#include "Components/BillboardComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldSky)

namespace MobWorldSkyParam
{
	/**
	 * The engine's HDRI backdrop material, whose names these are.
	 *
	 * Kept rather than renamed so a project can point SkyMaterial at the engine's own instance and
	 * have it work, which is what makes this a port of that actor rather than a lookalike.
	 */
	static const FName Cubemap = TEXT("HDRI_Map");
	static const FName Intensity = TEXT("Intensity");
	static const FName Angle = TEXT("HDRI_Angle");
	static const FName ProjectionPosition = TEXT("ProjectionPosition");
	static const FName UseCameraPosition = TEXT("UseCameraPosition");

	/** On the collection, read by anything that has to line up with the sky. */
	static const FName SkyYaw = TEXT("SkyYaw");
	static const FName SkyProjectionCenter = TEXT("SkyProjectionCenter");
}

namespace MobWorldSkyDefaults
{
	// A sphere rather than the HDRI Backdrop plugin's dome: the dome carries a ground half that only
	// earns its place when the floor is visible, and a sky with no visible floor is the common case.
	// Point DefaultMesh at EnviroDome for the ground projection.
	static const TCHAR* Dome = TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere");
	// MobWorld's own instances over the engine's projection masters, so a project has somewhere to
	// put an override without editing engine plugin content in place.
	static const TCHAR* SkyMaterial = TEXT("/MobWorld/Sky/MI_MobWorldSky.MI_MobWorldSky");
	static const TCHAR* FloorMaterial = TEXT("/MobWorld/Sky/MI_MobWorldFloor.MI_MobWorldFloor");

	/**
	 * The two material slots a backdrop mesh may carry, by name.
	 *
	 * By name because index order is the mesh's business. A mesh with neither name and a single slot
	 * is a plain sphere, and takes the sky material there.
	 */
	static const FName SkySlot = TEXT("Sky");
	static const FName FloorSlot = TEXT("Floor");

	/** Size is authored in metres because that is how far away a sky reads; the engine wants centimetres. */
	static constexpr float MetresToCentimetres = 100.f;
}

AMobWorldSky::AMobWorldSky()
{
	PrimaryActorTick.bCanEverTick = false;

	SkyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyMesh"));
	SetRootComponent(SkyMesh);

	// Drawn from the inside, so it casts nothing, occludes nothing and is never in the way of a
	// trace. Everything a sky sphere does wrong, it does through one of those.
	SkyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkyMesh->SetCollisionProfileName(TEXT("NoCollision"));
	SkyMesh->SetCastShadow(false);
	SkyMesh->bCastDynamicShadow = false;
	SkyMesh->SetMobility(EComponentMobility::Movable);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(SkyMesh);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
	SkyLight->bLowerHemisphereIsBlack = false;

	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
#if WITH_EDITORONLY_DATA
		MobWorldSprite::Apply(Sprite);
#endif
		Sprite->SetupAttachment(SkyMesh);
		Sprite->bIsScreenSizeScaled = true;
		Sprite->SetHiddenInGame(true);

		// The dome is scaled to the size of the level, and a billboard under it would inherit that.
		Sprite->SetUsingAbsoluteScale(true);
	}

	DefaultMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MobWorldSkyDefaults::Dome));
	SkyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(MobWorldSkyDefaults::SkyMaterial));
	FloorMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(MobWorldSkyDefaults::FloorMaterial));
	FortLighting = UMobWorldSettings::Get()->FortLighting;
}

FMobWorldSkyEntry AMobWorldSky::GetSky() const
{
	const UMobWorldSettings* Settings = UMobWorldSettings::Get();

	// The override first, so a level can show a sky the game is not otherwise standing under. Then
	// the world's, and then the settings' default.
	//
	// Read straight from the set rather than through the subsystem, so a backdrop draws the right
	// sky in a world that has no subsystem yet - which is every world during load, and any editor
	// world where one was never created. Depending on it made the backdrop blank with nothing on
	// screen to say which of the two was missing.
	int32 Index = SkyIndexOverride;
	if (Index < 0)
	{
		const UMobWorldSubsystem* World = UMobWorldSubsystem::Get(this);
		Index = World ? World->GetSkyIndex() : Settings->DefaultSkyIndex;
	}

	const UMobWorldSkySet* Set = Settings->SkySet.LoadSynchronous();
	const FMobWorldSkyEntry* Entry = Set ? Set->Find(Index) : nullptr;
	return Entry ? *Entry : FMobWorldSkyEntry();
}

float AMobWorldSky::GetSkyYaw() const
{
	return GetSky().Yaw;
}

void AMobWorldSky::RefreshSky()
{
	const FMobWorldSkyEntry Sky = GetSky();
	const float Radius = Sky.Size * MobWorldSkyDefaults::MetresToCentimetres;

	// Whatever is in memory. Null while the subsystem is still streaming the sky in, which is a
	// state to draw nothing for rather than an error: OnSkyChanged brings us back when it lands.
	UTextureCube* Cubemap = Sky.Cubemap.Get();

	if (SkyMesh)
	{
		SkyMesh->SetVisibility(Cubemap != nullptr);

		if (UStaticMesh* Mesh = Sky.Mesh.Get() ? Sky.Mesh.Get() : DefaultMesh.LoadSynchronous())
		{
			if (SkyMesh->GetStaticMesh() != Mesh)
			{
				SkyMesh->SetStaticMesh(Mesh);
			}
		}

		SkyMesh->SetWorldScale3D(FVector(Sky.Size));

		ApplyToSlot(MobWorldSkyDefaults::SkySlot, SkyMaterial, SkyMaterialInstance, Sky, Cubemap);
		ApplyToSlot(MobWorldSkyDefaults::FloorSlot, FloorMaterial, FloorMaterialInstance, Sky, Cubemap);
	}

	if (SkyLight)
	{
		SkyLight->SetVisibility(bCreateSkyLight && Cubemap != nullptr);

		if (bCreateSkyLight && Cubemap)
		{
			SkyLight->SetCubemap(Cubemap);
			SkyLight->SetIntensity(Sky.Intensity);
			SkyLight->SetLowerHemisphereColor(Sky.LowerHemisphereColor);
			SkyLight->bLowerHemisphereIsBlack = Sky.bLowerHemisphereIsBlack;

			// Where the sky light thinks the sky is. Left at the engine default it treats the dome as
			// scenery to be lit rather than as the thing doing the lighting.
			SkyLight->SkyDistanceThreshold = FMath::Max(Radius * Sky.LightingDistanceFactor, 1.f);
			SkyLight->MarkRenderStateDirty();
			SkyLight->RecaptureSky();
		}
	}

	if (bDrivesMaterials)
	{
		WriteCollection(Sky.Yaw);
	}
}

void AMobWorldSky::ApplyToSlot(const FName SlotName, const TSoftObjectPtr<UMaterialInterface>& Source,
	TObjectPtr<UMaterialInstanceDynamic>& Instance, const FMobWorldSkyEntry& Sky,
	UTextureCube* Cubemap)
{
	// By name rather than by index. The dome's slots are Sky and Floor, and writing index 0 put the
	// sky projection on the ground half and left the sky showing the material's own placeholder.
	int32 SlotIndex = SkyMesh->GetMaterialIndex(SlotName);

	// A mesh that names no slots is a plain sphere with one material on it. The sky goes there; the
	// floor has nowhere to go and is skipped.
	if (SlotIndex == INDEX_NONE && SlotName == MobWorldSkyDefaults::SkySlot
		&& SkyMesh->GetNumMaterials() == 1)
	{
		SlotIndex = 0;
	}

	UMaterialInterface* Material = Source.LoadSynchronous();

	if (SlotIndex == INDEX_NONE || !Material || !Cubemap)
	{
		return;
	}

	if (!Instance || Instance->Parent != Material)
	{
		Instance = UMaterialInstanceDynamic::Create(Material, this);
		SkyMesh->SetMaterial(SlotIndex, Instance);
	}

	Instance->SetTextureParameterValue(MobWorldSkyParam::Cubemap, Cubemap);
	Instance->SetScalarParameterValue(MobWorldSkyParam::Intensity, Sky.Intensity);
	Instance->SetScalarParameterValue(MobWorldSkyParam::Angle, Sky.Yaw);
	Instance->SetScalarParameterValue(MobWorldSkyParam::UseCameraPosition,
		Sky.bUseCameraProjection ? 1.f : 0.f);
	Instance->SetVectorParameterValue(MobWorldSkyParam::ProjectionPosition,
		FLinearColor(GetProjectionCenter()));
}

FVector AMobWorldSky::GetProjectionCenter() const
{
	return GetActorTransform().TransformPosition(ProjectionCenter);
}

void AMobWorldSky::WriteCollection(const float Yaw) const
{
	if (FortLighting.IsNull())
	{
		return;
	}

	const UWorld* World = GetWorld();
	UMaterialParameterCollection* Collection = FortLighting.LoadSynchronous();
	if (!World || !Collection)
	{
		return;
	}

	if (UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(Collection))
	{
		Instance->SetScalarParameterValue(MobWorldSkyParam::SkyYaw, Yaw);
		Instance->SetVectorParameterValue(MobWorldSkyParam::SkyProjectionCenter,
			FLinearColor(GetProjectionCenter()));
	}

	// And into the surface collection, because a wall reflecting the sky has to turn with it. The
	// two plugins keep their own copy of the parameter rather than sharing one, so both are written.
	UMaterialParameterCollection* Surfaces =
		UMobWorldSettings::Get()->WeatherCollection.LoadSynchronous();

	if (Surfaces && Surfaces != Collection)
	{
		if (UMaterialParameterCollectionInstance* Instance =
			World->GetParameterCollectionInstance(Surfaces))
		{
			Instance->SetScalarParameterValue(MobWorldSkyParam::SkyYaw, Yaw);
		}
	}
}

void AMobWorldSky::RefreshAll()
{
	if (!GEngine)
	{
		return;
	}

	// Every live world, because the editor's and a running play session's both have backdrops in
	// them and editing the set should move both.
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World)
		{
			continue;
		}

		for (TActorIterator<AMobWorldSky> It(World); It; ++It)
		{
			It->RefreshSky();
		}
	}
}

void AMobWorldSky::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshSky();
}

void AMobWorldSky::BeginPlay()
{
	Super::BeginPlay();

	// Again on begin play, because the collection is per world and the construction script ran
	// against the editor's copy of it.
	RefreshSky();
}

#if WITH_EDITOR
void AMobWorldSky::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshSky();
}
#endif
