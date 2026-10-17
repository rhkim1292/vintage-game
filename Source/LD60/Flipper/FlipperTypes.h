// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperItemDefinition.h"
#include "FlipperTypes.generated.h"

/**
 *  One concrete item in the world: a unique id plus the kind of item it is.
 *  Phase 3 adds the rolled properties (rarity, condition, traits, Untested).
 */
USTRUCT(BlueprintType)
struct FFlipperItemInstance
{
	GENERATED_BODY()

	/** Unique per item instance. Two items with the same Id are the same item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int32 Id = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TObjectPtr<UFlipperItemDefinition> Definition;
};

/** Where an item sits in a grid: its top-left cell, and whether it is turned a quarter turn. */
USTRUCT(BlueprintType)
struct FFlipperPlacement
{
	GENERATED_BODY()

	/** Top-left cell of the footprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
	FIntPoint Origin = FIntPoint::ZeroValue;

	/** When true the footprint is Height wide and Width tall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
	bool bRotated = false;
};

/** An item together with its placement in one particular grid. */
USTRUCT(BlueprintType)
struct FFlipperPlacedItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Placement")
	FFlipperItemInstance Item;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Placement")
	FFlipperPlacement Placement;
};
