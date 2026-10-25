// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperTypes.h"
#include "FlipperPricing.generated.h"

/** How the buyer feels today (Docs/FlipperSpec.md, Feature 5). One mood per day scales every offer. */
UENUM(BlueprintType)
enum class EFlipperBuyerMood : uint8
{
	Slow,
	Normal,
	Hot,
};

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

	/** ...times the multiplier of the item's condition (Docs/FlipperSpec.md, Feature 7). Good is the baseline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float PoorMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float FairMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float GoodMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pricing", meta = (ClampMin = 0))
	float MintMultiplier = 1.3f;

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

	/** The buyer pays this fraction of Value for an Untested item, whether or not it works (Feature 5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float UntestedOfferFraction = 0.5f;

	/** The offer multiplier of each buyer mood. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float SlowMoodMultiplier = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float NormalMoodMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float HotMoodMultiplier = 1.25f;

	/** How often each mood is rolled, relative to the others (25 / 50 / 25 means Normal is twice as likely as Slow or Hot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float SlowMoodWeight = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float NormalMoodWeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selling", meta = (ClampMin = 0))
	float HotMoodWeight = 25.f;
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

	/** The condition's multiplier from the config. */
	inline float GetConditionMultiplier(EFlipperCondition Condition, const FFlipperPricingConfig& Config)
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return Config.PoorMultiplier;
		case EFlipperCondition::Fair:
			return Config.FairMultiplier;
		case EFlipperCondition::Mint:
			return Config.MintMultiplier;
		default:
			return Config.GoodMultiplier;
		}
	}

	/**
	 *  The product of the ValueMultiplier of every trait on the item. An item with no traits gives 1. A null entry in
	 *  Traits is skipped (counts as 1).
	 */
	float GetTraitMultiplier(const FFlipperItemInstance& Item);

	/**
	 *  What a WORKING copy of this item is worth: the definition's BaseValue times the rarity multiplier, times the
	 *  condition multiplier, times GetTraitMultiplier, rounded ONCE at the end to the nearest whole dollar
	 *  (FMath::RoundToInt). Does not depend on bTested or bWorks.
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

	/** The multiplier of this mood from the config. */
	float GetMoodMultiplier(EFlipperBuyerMood Mood, const FFlipperPricingConfig& Config);

	/** Rolls a mood by the config's weights. Takes exactly one number from Random. */
	EFlipperBuyerMood RollMood(const FFlipperPricingConfig& Config, FRandomStream& Random);

	/**
	 *  What the buyer pays for this item (Docs/FlipperSelling-design.md). The buyer pays for what is KNOWN:
	 *    - Tested and working: the Value.
	 *    - Tested and dead:    the scrap value.
	 *    - Untested:           Value x UntestedOfferFraction, whether or not it works (the buyer does not open it).
	 *  The result is that base times MoodMultiplier, rounded once to the nearest whole dollar, never below 1 for an
	 *  item with a Definition. An item with no Definition gets 0. It ignores AskPrice and PaidPrice.
	 */
	int32 GetOffer(const FFlipperItemInstance& Item, const FFlipperPricingConfig& Config, float MoodMultiplier);
} // namespace FlipperPricing
