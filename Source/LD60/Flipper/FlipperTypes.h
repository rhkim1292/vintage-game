// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperItemDefinition.h"
#include "FlipperTypes.generated.h"

/** Rolled per item. Three tiers, weights in FFlipperGenerationConfig (Docs/FlipperSpec.md, Feature 2). */
UENUM(BlueprintType)
enum class EFlipperRarity : uint8
{
	Common,
	Uncommon,
	Rare
};

namespace FlipperRarity
{
	/** Border and name colour: grey, blue, gold. */
	inline FLinearColor GetColor(EFlipperRarity Rarity)
	{
		switch (Rarity)
		{
		case EFlipperRarity::Uncommon:
			return FLinearColor(0.3f, 0.55f, 1.f, 1.f);
		case EFlipperRarity::Rare:
			return FLinearColor(1.f, 0.8f, 0.2f, 1.f);
		default:
			return FLinearColor(0.65f, 0.65f, 0.65f, 1.f);
		}
	}

	inline const TCHAR* GetName(EFlipperRarity Rarity)
	{
		switch (Rarity)
		{
		case EFlipperRarity::Uncommon:
			return TEXT("Uncommon");
		case EFlipperRarity::Rare:
			return TEXT("Rare");
		default:
			return TEXT("Common");
		}
	}
} // namespace FlipperRarity

/**
 *  One concrete item in the world: a unique id plus the kind of item it is.
 *  Rarity, bTested and bWorks are rolled when the item is generated (Docs/FlipperSpec.md, Feature 2).
 *  The defaults describe a plain, known-working Common item, which is what hand-made test items get.
 */
USTRUCT(BlueprintType)
struct FFlipperItemInstance
{
	GENERATED_BODY()

	/** Unique per item instance. Two items with the same Id are the same item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Id = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UFlipperItemDefinition> Definition;

	/** Rolled per item: two Game Boys can differ. Sets the border colour and a price multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EFlipperRarity Rarity = EFlipperRarity::Common;

	/**
	 *  True when the player knows whether the item works: it was Tested on the seller table (always working),
	 *  or an Untested one has been tested in the Garage. False = Untested, bWorks is hidden from the player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bTested = true;

	/** The hidden truth. Only ever false on an item that was rolled Untested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bWorks = true;
};

/** Where an item sits in a grid: its top-left cell, and whether it is turned a quarter turn. */
USTRUCT(BlueprintType)
struct FFlipperPlacement
{
	GENERATED_BODY()

	/** Top-left cell of the footprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	FIntPoint Origin = FIntPoint::ZeroValue;

	/** When true the footprint is Height wide and Width tall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
	bool bRotated = false;
};

/** An item together with its placement in one particular grid. */
USTRUCT(BlueprintType)
struct FFlipperPlacedItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	FFlipperItemInstance Item;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement")
	FFlipperPlacement Placement;
};
