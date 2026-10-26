// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlipperCollectorDefinition.generated.h"

/**
 *  Static description of one collector (Docs/FlipperCollectors-design.md): the name on the card. It has no gameplay
 *  effect; a collector's request is rolled each day from the seller table. A portrait can be added here later.
 */
UCLASS(BlueprintType)
class UFlipperCollectorDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Name shown on the collector's card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collector")
	FText DisplayName;
};
