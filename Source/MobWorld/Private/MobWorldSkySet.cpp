// Copyright (c) Jared Taylor

#include "MobWorldSkySet.h"

#include "MobWorldSky.h"
#include "MobWorldSubsystem.h"

#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MobWorldSkySet)

#if WITH_EDITOR
void UMobWorldSkySet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	PushToWorlds();
}

void UMobWorldSkySet::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	// Both overloads, because a field inside an array element arrives here and not at the one above.
	// Every property on a sky is one of those, so without this the only edits that propagated were
	// adding and removing whole entries.
	PushToWorlds();
}

void UMobWorldSkySet::PushToWorlds() const
{
	// Straight out to whatever is showing this, so turning a sky or changing its brightness is
	// something you watch happen rather than something you save and reload to see.
	AMobWorldSky::RefreshAll();
	UMobWorldSubsystem::RefreshAllWorlds();
}
#endif
