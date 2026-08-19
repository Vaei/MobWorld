// Copyright (c) Jared Taylor

#include "MobWorldTypes.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

void MobWorldSprite::Apply(UBillboardComponent* Sprite)
{
	if (!Sprite)
	{
		return;
	}

	static ConstructorHelpers::FObjectFinderOptional<UTexture2D> Icon(
		TEXT("/MobWorld/Textures/T_MobWorldSprite.T_MobWorldSprite"));

	if (UTexture2D* Texture = Icon.Get())
	{
		Sprite->SetSprite(Texture);
	}

	// Twice the engine's default. These mark rooms and skies rather than single points, so they are
	// looked for across a level rather than found where you already are.
	Sprite->ScreenSize = 0.005f;
}
#endif
