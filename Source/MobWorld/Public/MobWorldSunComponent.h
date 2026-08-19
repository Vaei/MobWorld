// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/DirectionalLightComponent.h"
#include "MobWorldSunComponent.generated.h"

class UMaterialParameterCollection;

/**
 * The sun, and the only thing that tells the unlit shading where it is.
 *
 * MobFort's masters are Unlit, so the engine never hands them a light. Everything they shade against
 * lives in a collection, and until something writes it a character is lit from whatever direction
 * was last saved into that collection's defaults - which looks like a lighting bug in the character
 * rather than a light nobody wired up.
 *
 * Pushed on register, on movement and on any render state change, so a sun that never moves costs
 * nothing and one driven from Blueprint needs no tick: colour and intensity reach the renderer by
 * marking the render state dirty, which is exactly where this catches them.
 *
 * Use it in place of a plain directional light. It is the light, not a component beside one.
 */
UCLASS(ClassGroup=(Mob), meta=(BlueprintSpawnableComponent))
class MOBWORLD_API UMobWorldSunComponent : public UDirectionalLightComponent
{
	GENERATED_BODY()

public:
	UMobWorldSunComponent();

	/** The collection the unlit masters read. Cleared, this light feeds nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mob World")
	TSoftObjectPtr<UMaterialParameterCollection> FortLighting;

	/** Written as the direction toward this light, with w left at zero. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mob World", AdvancedDisplay)
	FName SunDirectionParameter = TEXT("SunDirection");

	/**
	 * Written as this light's colour.
	 *
	 * Alpha is read back and put where it was. It is the collection's own sun intensity, hand set
	 * against an unlit master with no exposure, and a light actor's lux is not that number: copying
	 * it across blows every character out the moment somebody brightens the level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mob World", AdvancedDisplay)
	FName SunColorParameter = TEXT("SunColor");

	/** Writes this light into the collection now. */
	UFUNCTION(BlueprintCallable, Category="Mob World")
	void SyncFortLighting();

protected:
	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void SendRenderTransform_Concurrent() override;
	//~ End UActorComponent Interface
};
