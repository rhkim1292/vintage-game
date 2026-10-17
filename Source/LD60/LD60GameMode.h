// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LD60GameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ALD60GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ALD60GameMode();
};



