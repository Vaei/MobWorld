// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MobWorldTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "MobWorldSubsystem.generated.h"

class AMobWorldSky;
class UMaterialInstanceDynamic;
class UMobWorldSkySet;
class UPrimitiveComponent;
class UTextureCube;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMobWorldOnSkyChanged, int32, SkyIndex);

/**
 * What the world is doing, and everything that has to be told about it.
 *
 * The Mob plugins are written to stand alone, so none of them knows about any of the others: each
 * has a collection, a component or a texture parameter that something has to fill. This is the
 * something. A project says which sky and what the weather is; where those answers come from - a
 * save, a mission, a level, a cheat - is the project's business and deliberately not modelled here.
 *
 * On the world rather than the game instance because everything it writes is per world. A material
 * parameter collection resets to its saved defaults on travel, and a material instance belongs to a
 * level's actors, so state kept anywhere longer-lived would be state that quietly stops applying.
 */
UCLASS(BlueprintType)
class MOBWORLD_API UMobWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @return The subsystem for a world, or null outside one.
	 *
	 * Blueprint has its own Get Mob World Subsystem node from the class being BlueprintType; this is
	 * for C++ and for anything holding a world context rather than a world.
	 */
	static UMobWorldSubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ End USubsystem Interface

	/** Fired after a sky has been applied, including the first time a world is put under one. */
	UPROPERTY(BlueprintAssignable, Category="MobWorld")
	FMobWorldOnSkyChanged OnSkyChanged;

	//~ The sky ---------------------------------------------------------------------------------

	/** @return Which sky the world is under. */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	int32 GetSkyIndex() const { return SkyIndex; }

	/** @return The sky itself, or an empty one when the index names nothing. */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	FMobWorldSkyEntry GetSky() const;

	/**
	 * @return The cubemap of the standing sky, or null while it is still loading.
	 *
	 * Null is a state, not a failure. The sky's assets are soft and loaded on demand, so anything
	 * drawing one has to cope with not having it yet and be told again when it arrives - which is
	 * what OnSkyChanged is for.
	 */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	UTextureCube* GetSkyCubemap() const;

	/** @return Whether the standing sky's assets are in memory. */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	bool IsSkyLoaded() const;

	/**
	 * Starts loading a sky without showing it.
	 *
	 * For a change you can see coming: call it while the player is still somewhere else, and the
	 * SetSkyIndex that follows has nothing left to wait for. Without it a sky change is a frame or
	 * several of the old sky, which is fine across a level load and not fine mid-scene.
	 *
	 * One at a time. Preloading a second sky drops the first, on the grounds that a caller who has
	 * changed their mind about what is coming next is not still waiting on the old answer.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Sky")
	void PreloadSky(int32 Index);

	/** @return Whether a preloaded sky is in memory and ready to be shown without a wait. */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	bool IsSkyPreloaded(int32 Index) const;

	/**
	 * Puts the world under a sky.
	 *
	 * Reaches every material already handed out, not only the ones created afterwards. An index that
	 * names nothing is still taken, because a project stepping past the end of its own list wants to
	 * see that rather than have it quietly ignored.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Sky")
	void SetSkyIndex(int32 NewIndex);

	/**
	 * How hard the sky reflects, overriding what the sky says for itself.
	 *
	 * Below zero clears the override and hands every sky back its own answer.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Sky")
	void SetSpecularOverride(float Specular);

	/** @return The reflection strength in force: the override, or the sky's own. */
	UFUNCTION(BlueprintPure, Category="MobWorld|Sky")
	float GetSpecular() const;

	//~ The weather -----------------------------------------------------------------------------

	/**
	 * How wet the world is and how much snow is on it, both nought to one.
	 *
	 * Written straight into MobMaterials' collection, which every surface master reads. What decides
	 * these numbers - a weather enum, a forecast, a cutscene - is the project's.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Weather")
	void SetWeather(float Wetness, float Snow, float PuddleAmount);

	//~ Materials -------------------------------------------------------------------------------

	/**
	 * Makes a component's materials dynamic, puts them under the standing sky, and remembers them.
	 *
	 * Remembering is the point. Left to each owner to bind a delegate for, an owner that forgets
	 * keeps the sky it spawned under for the rest of its life and nothing says so.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Material")
	void RegisterComponent(UPrimitiveComponent* Component,
		TArray<UMaterialInstanceDynamic*>& OutInstances, FGameplayTag GradientTag = FGameplayTag());

	/**
	 * The same for every mesh on an actor, optionally only those carrying a mesh tag.
	 *
	 * GradientTag says which of the sky's gradient atlases this actor is lit through. Left empty,
	 * which is the whole of it for a project not using them, it takes the sky's untagged one.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Material")
	void RegisterActor(AActor* Actor, TArray<UMaterialInstanceDynamic*>& OutInstances,
		FName MeshTag = NAME_None, FGameplayTag GradientTag = FGameplayTag());

	/** Puts instances made elsewhere under the sky, and follows them from now on. */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Material")
	void RegisterMaterials(const TArray<UMaterialInstanceDynamic*>& Instances,
		FGameplayTag GradientTag = FGameplayTag());

	/**
	 * Moves already-registered instances onto a different gradient.
	 *
	 * For a character whose gradients follow something that changes at run time, an outfit out of an
	 * inventory being the case this exists for.
	 */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Material")
	void SetGradientTag(const TArray<UMaterialInstanceDynamic*>& Instances, FGameplayTag GradientTag);

	/** Writes the standing sky over everything registered, dropping whatever has been destroyed. */
	UFUNCTION(BlueprintCallable, Category="MobWorld|Material")
	void RefreshMaterials();

	/** Rewrites every live world's materials. What the sky set calls when it is edited. */
	static void RefreshAllWorlds();

protected:
	/** Writes the sky into the collections, the sky spheres and the materials. All of it. */
	void ApplySky();

	/** Puts one instance under the standing sky, through the gradients its tag names. */
	void ApplySkyToMaterial(UMaterialInstanceDynamic* Instance, FGameplayTag GradientTag) const;

	/** Tells the sky spheres in the level to redraw. */
	void RefreshSkyActors() const;

	/** @return The set the settings name, loaded. */
	const UMobWorldSkySet* GetSkySet() const;

	/**
	 * Starts loading the standing sky, and applies it once it is in.
	 *
	 * Asynchronous, because a sky is several large textures and a level change is the moment a hitch
	 * is least welcome. The previous handle is released first, so walking through skies does not
	 * hold every one it passed.
	 */
	void RequestSky();

	/** Handed to the streamer when the load finishes. */
	void OnSkyLoaded();

	/** Keeps the standing sky's assets in memory, and nothing else's. */
	TSharedPtr<FStreamableHandle> SkyHandle;

	/** Keeps a sky that is expected but not yet shown. Handed over to SkyHandle when it is. */
	TSharedPtr<FStreamableHandle> PreloadHandle;

	/** Which sky PreloadHandle is holding, so setting that one can reuse it. */
	int32 PreloadedIndex = INDEX_NONE;

	/** Which sky the world is under. Starts at the settings' default. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MobWorld")
	int32 SkyIndex = 0;

	/** Below zero is no override, which hands every sky its own answer. */
	float SpecularOverride = -1.f;

	/** One registered instance, and which of the sky's gradients it is lit through. */
	struct FTrackedInstance
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Instance;
		FGameplayTag GradientTag;
	};

	/**
	 * Every instance put under a sky, so a change reaches all of them.
	 *
	 * Weak, and swept as it is walked: the characters holding these come and go constantly, and a
	 * list only ever appended to grows for the length of the level.
	 */
	TArray<FTrackedInstance> TrackedInstances;
};
