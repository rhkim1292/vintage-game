// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FlipperItemDefinition.generated.h"

class UTexture2D;

/**
 *  Static description of one kind of item (e.g. "Game Boy").
 *  Holds the grid footprint, the base value, how likely it is to appear on the seller table, its category tags
 *  and whether it is a lot (Docs/FlipperSpec.md, Features 2 and 7). Rarity, condition, traits and the works/dead
 *  flag are rolled per item, not stored here.
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

	/** Price of a Common, Good-condition, working copy in dollars. The pricing rules scale this by rarity, condition and traits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = 1))
	int32 BaseValue = 10;

	/** How likely this item is to be rolled onto the seller table, relative to the other definitions. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation", meta = (ClampMin = 0))
	float SpawnWeight = 1.f;

	/** Icon drawn inside the item's footprint */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	/** What kind of item this is (Item.Category.Handheld, ...). Collector requests match on these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (Categories = "Item.Category"))
	FGameplayTagContainer Categories;

	/** A lot is a bundle of identical small items sold as one item (e.g. a box of floppies). It rolls like any other item. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lot")
	bool bIsLot = false;

	/** How many items the lot holds. Only used for the tooltip text "Lot of N". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lot", meta = (ClampMin = 1, EditCondition = "bIsLot"))
	int32 LotCount = 10;
};
