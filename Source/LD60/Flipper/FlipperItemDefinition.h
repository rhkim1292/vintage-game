// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FlipperItemDefinition.generated.h"

class UTexture2D;

/**
 *  Static description of one kind of item (e.g. "Game Boy").
 *  Holds the grid footprint, the base value and how likely it is to appear on the seller table
 *  (Docs/FlipperSpec.md, Feature 2). Rarity and the works/dead flag are rolled per item, not stored here.
 */
UCLASS(BlueprintType)
class UFlipperItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Name shown in the tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** Footprint width in grid cells when not rotated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 Width = 1;

	/** Footprint height in grid cells when not rotated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 Height = 1;

	/** Price of a Common, working copy in dollars. The pricing rules (Phase 4) scale this by rarity and condition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 BaseValue = 10;

	/** How likely this item is to be rolled onto the seller table, relative to the other definitions. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = 0))
	float SpawnWeight = 1.f;

	/** Icon drawn inside the item's footprint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;
};
