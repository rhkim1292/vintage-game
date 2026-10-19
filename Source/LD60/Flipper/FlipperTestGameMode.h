// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FlipperTestGameMode.generated.h"

/**
 *  Phase 2.2 debug game mode: no pawn, uses AFlipperTestPlayerController to show the trunk and seller table.
 *  Set it as the GameMode Override in the test level's World Settings.
 */
UCLASS()
class AFlipperTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFlipperTestGameMode();
};
