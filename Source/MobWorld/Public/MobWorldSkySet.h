// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MobWorldTypes.h"
#include "MobWorldSkySet.generated.h"

/**
 * Every sky a project can be under.
 *
 * A game picks one by its position here, so the order is a contract: anything that remembers a sky
 * remembers the number, and inserting one in the middle moves every answer already written down.
 * Append.
 */
UCLASS(BlueprintType)
class MOBWORLD_API UMobWorldSkySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sky", meta=(TitleProperty="Cubemap"))
	TArray<FMobWorldSkyEntry> Skies;

	/** @return The sky at an index, or null past the end. */
	const FMobWorldSkyEntry* Find(int32 Index) const
	{
		return Skies.IsValidIndex(Index) ? &Skies[Index] : nullptr;
	}

#if WITH_EDITOR
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	/** Redraws every backdrop and rewrites every registered material. */
	void PushToWorlds() const;
#endif
};
