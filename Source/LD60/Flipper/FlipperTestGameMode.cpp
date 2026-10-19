// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperTestGameMode.h"
#include "FlipperTestPlayerController.h"

AFlipperTestGameMode::AFlipperTestGameMode()
{
	PlayerControllerClass = AFlipperTestPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
