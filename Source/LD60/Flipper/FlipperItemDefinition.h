// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlipperItemDefinition.generated.h"

class UTexture2D;

/**
 *  Static description of one kind of item (e.g. "Game Boy").
 *  Phase 1 only needs what the grid cares about; rarity, traits and pricing fields are added in Phase 3.
 */
UCLASS(BlueprintType)
class UFlipperItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Name shown in the tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	/** Footprint width in grid cells when not rotated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta = (ClampMin = 1))
	int32 Width = 1;

	/** Footprint height in grid cells when not rotated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta = (ClampMin = 1))
	int32 Height = 1;

	/** Icon drawn inside the item's footprint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UTexture2D> Icon;
};
