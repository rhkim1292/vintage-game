// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperItemDefinition.h"
#include "FlipperTraitDefinition.h"
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

/** Rolled per item. A visible grade that sets a price multiplier and the works chance of an Untested item (Docs/FlipperSpec.md, Feature 7). */
UENUM(BlueprintType)
enum class EFlipperCondition : uint8
{
	Poor,
	Fair,
	Good,
	Mint
};

namespace FlipperCondition
{
	/** Tooltip word colour and grid badge colour: red, amber, green, cyan. */
	inline FLinearColor GetColor(EFlipperCondition Condition)
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return FLinearColor(1.f, 0.35f, 0.3f, 1.f);
		case EFlipperCondition::Fair:
			return FLinearColor(1.f, 0.7f, 0.25f, 1.f);
		case EFlipperCondition::Mint:
			return FLinearColor(0.35f, 0.95f, 0.95f, 1.f);
		default:
			return FLinearColor(0.4f, 0.9f, 0.45f, 1.f);
		}
	}

	inline const TCHAR* GetName(EFlipperCondition Condition)
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return TEXT("Poor");
		case EFlipperCondition::Fair:
			return TEXT("Fair");
		case EFlipperCondition::Mint:
			return TEXT("Mint");
		default:
			return TEXT("Good");
		}
	}

	/** The one-letter badge drawn on the grid: P, F, G or M. */
	inline const TCHAR* GetBadge(EFlipperCondition Condition)
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return TEXT("P");
		case EFlipperCondition::Fair:
			return TEXT("F");
		case EFlipperCondition::Mint:
			return TEXT("M");
		default:
			return TEXT("G");
		}
	}
} // namespace FlipperCondition

/**
 *  One concrete item in the world: a unique id plus the kind of item it is.
 *  Rarity, Condition, Traits, bTested and bWorks are rolled when the item is generated (Docs/FlipperSpec.md, Features 2 and 7).
 *  The defaults describe a plain, known-working Common, Good-condition item with no traits, which is what hand-made
 *  test items get.
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

	/** Rolled per item and always visible. Sets a price multiplier and, for an Untested item, the chance that it works. Good is the baseline (x1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EFlipperCondition Condition = EFlipperCondition::Good;

	/** Zero, one or two distinct traits, always visible. Their multipliers are multiplied into the item's Value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TArray<TObjectPtr<UFlipperTraitDefinition>> Traits;

	/**
	 *  True when the player knows whether the item works: it was Tested on the seller table (always working),
	 *  or an Untested one has been tested in the Garage. False = Untested, bWorks is hidden from the player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bTested = true;

	/** The hidden truth. Only ever false on an item that was rolled Untested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bWorks = true;

	/** What the seller charges. Rolled with the item (Docs/FlipperSpec.md, Feature 3). 0 on hand-made test items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 AskPrice = 0;

	/** What the player paid when buying it. A refund returns exactly this. 0 until bought. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 PaidPrice = 0;

	/** True from the moment it is bought until the Market phase ends: only then may it go back to the table for a refund. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bBoughtThisPhase = false;
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
