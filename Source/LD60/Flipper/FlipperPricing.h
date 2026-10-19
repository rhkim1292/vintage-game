// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperTypes.h"
#include "FlipperPricing.generated.h"

/**
 *  Every number the prices are made from. Kept in data (not in code) so the balance pass can tune it without
 *  recompiling. Defaults are the first guesses from Docs/FlipperSpec.md, Feature 3.
 */
USTRUCT(BlueprintType)
struct FFlipperPricingConfig
{
	GENERATED_BODY()

	/** Value = the definition's BaseValue times the multiplier of the item's rarity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float CommonMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float UncommonMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float RareMultiplier = 4.f;

	/** A dead item is worth this fraction of its Value (scrap). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0, ClampMax = 1))
	float ScrapFraction = 0.1f;

	/** The seller's ask is this fraction of Value for a Tested item (before the variation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float TestedAskFraction = 0.75f;

	/** ...and this fraction for an Untested item (the seller does not know if it works). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float UntestedAskFraction = 0.5f;

	/** Each ask is multiplied by a random number between 1 - AskVariation and 1 + AskVariation. 0.2 = plus or minus 20 percent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0, ClampMax = 1))
	float AskVariation = 0.2f;

	/** Cash the player starts the run with. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	int32 StartingCash = 200;
};

/**
 *  The money formulas (Docs/FlipperSpec.md, Feature 3). Pure functions: no state, no widgets. All money is whole
 *  dollars. An item with no Definition is worth 0 in every function.
 */
namespace FlipperPricing
{
	/** The rarity's multiplier from the config. */
	inline float GetRarityMultiplier(EFlipperRarity Rarity, const FFlipperPricingConfig& Config)
	{
		switch (Rarity)
		{
		case EFlipperRarity::Uncommon:
			return Config.UncommonMultiplier;
		case EFlipperRarity::Rare:
			return Config.RareMultiplier;
		default:
			return Config.CommonMultiplier;
		}
	}

	/**
	 *  What a WORKING copy of this item is worth: the definition's BaseValue times the rarity multiplier,
	 *  rounded to the nearest whole dollar (FMath::RoundToInt). Does not depend on bTested or bWorks.
	 */
	int32 GetValue(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config);

	/**
	 *  What a DEAD copy is worth: ScrapFraction of its Value (the rounded Value from GetValue), rounded to the
	 *  nearest whole dollar. Does not depend on bWorks: it answers "what would it be worth if it were dead".
	 */
	int32 GetScrapValue(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config);

	/**
	 *  What the seller asks for this item: Value times TestedAskFraction (if Item.bTested) or UntestedAskFraction
	 *  (if not), times a random number between 1 - AskVariation and 1 + AskVariation, rounded to the nearest
	 *  whole dollar, never below 1 for an item with a Definition. It never looks at bWorks: the seller does not
	 *  know. Takes exactly one number from Random so a fixed seed gives the same asks every run.
	 */
	int32 GetAskPrice(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config, FRandomStream& Random);
} // namespace FlipperPricing
