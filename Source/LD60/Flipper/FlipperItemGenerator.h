// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
#include "FlipperPricing.h"
#include "FlipperItemGenerator.generated.h"

class UInventoryGrid;

/**
 *  Every number the generator rolls with. Kept in data (not in code) so the balance pass can tune it without
 *  recompiling. Defaults are the first guesses from Docs/FlipperSpec.md, Feature 2.
 */
USTRUCT(BlueprintType)
struct FFlipperGenerationConfig
{
	GENERATED_BODY()

	/** How many items are rolled onto the seller table each day. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	int32 ItemsPerDay = 8;

	/** Relative chances of each rarity (they do not have to add up to 100). The defaults give 70% / 25% / 5%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float CommonWeight = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float UncommonWeight = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float RareWeight = 5.f;

	/** Chance (0 to 1) that a rolled item is Untested rather than Tested. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float UntestedChance = 0.6f;

	/** Relative chances of each condition (they do not have to add up to 100). The defaults give 20% / 35% / 30% / 15%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float PoorWeight = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float FairWeight = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float GoodWeight = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float MintWeight = 15.f;

	/** Chance (0 to 1) that an Untested item of this condition turns out to work. The rest are dead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float PoorWorksChance = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float FairWorksChance = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float GoodWorksChance = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float MintWorksChance = 0.9f;

	/** Relative chances of an item having 0, 1 or 2 traits. The defaults give 55% / 35% / 10%. All three at 0 means no traits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float NoTraitsWeight = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float OneTraitWeight = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0))
	float TwoTraitsWeight = 10.f;

	/** The roll weight of this condition. */
	float GetConditionWeight(EFlipperCondition Condition) const
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return PoorWeight;
		case EFlipperCondition::Fair:
			return FairWeight;
		case EFlipperCondition::Mint:
			return MintWeight;
		default:
			return GoodWeight;
		}
	}

	/** The chance that an Untested item of this condition works. */
	float GetWorksChance(EFlipperCondition Condition) const
	{
		switch (Condition)
		{
		case EFlipperCondition::Poor:
			return PoorWorksChance;
		case EFlipperCondition::Fair:
			return FairWorksChance;
		case EFlipperCondition::Mint:
			return MintWorksChance;
		default:
			return GoodWorksChance;
		}
	}
};

/**
 *  Rolls items and fills the seller table (Docs/FlipperSpec.md, Feature 2). Pure logic: no widgets, no money.
 *  Every roll takes the caller's FRandomStream, so a test with a fixed seed gets the same result every run
 *  and the game can seed one stream per day.
 *
 *  One generator hands out the item Ids for a whole run (Ids must be unique across the trunk and the table),
 *  so keep one generator alive for the run and make every item with it.
 */
UCLASS(BlueprintType)
class UFlipperItemGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 *  Sets what can be rolled and with which numbers, and restarts the Id counter at 1. InTraits are the traits an
	 *  item can roll; leave it empty and no item ever gets a trait.
	 */
	void Init(const TArray<UFlipperItemDefinition*>& InDefinitions, const FFlipperGenerationConfig& InConfig, const FFlipperPricingConfig& InPricing = FFlipperPricingConfig(), const TArray<UFlipperTraitDefinition*>& InTraits = TArray<UFlipperTraitDefinition*>());

	const FFlipperGenerationConfig& GetConfig() const { return Config; }

	/**
	 *  Picks a rarity using the three weights in the config. A weight of 0 means that rarity never comes up.
	 *  If all three weights are 0, returns Common.
	 */
	EFlipperRarity RollRarity(FRandomStream& Random) const;

	/**
	 *  Picks one of the definitions given to Init, weighted by each definition's SpawnWeight (0 = never).
	 *  Returns nullptr if there are no definitions, or none has a weight above 0.
	 */
	UFlipperItemDefinition* RollDefinition(FRandomStream& Random) const;

	/**
	 *  Picks a condition using the four condition weights in the config. A weight of 0 means that condition never
	 *  comes up. If all four weights are 0, returns Good. Takes exactly one number from Random.
	 */
	EFlipperCondition RollCondition(FRandomStream& Random) const;

	/**
	 *  Picks the traits of one item. First it decides how many (0, 1 or 2) by NoTraitsWeight, OneTraitWeight and
	 *  TwoTraitsWeight (all three at 0 means none), then it picks that many DISTINCT traits from the ones given to
	 *  Init, weighted by each trait's RollWeight (0 = never; null entries never count). If fewer distinct traits can
	 *  be picked than the count, it returns as many as there are, so no traits given to Init gives an empty array.
	 *  Takes one number from Random for the count, then one more per trait it returns.
	 */
	TArray<TObjectPtr<UFlipperTraitDefinition>> RollTraits(FRandomStream& Random) const;

	/**
	 *  Rolls one complete item with the next unused Id (1, 2, 3, ...), in this order: a definition, a rarity, a
	 *  condition, the Tested / Untested state (and the works/dead flag), the traits, and the seller's AskPrice
	 *  (FlipperPricing::GetAskPrice, rolled last). A lot rolls exactly like any other item.
	 *    - Untested with chance UntestedChance. An Untested item has bTested = false, and bWorks is true with
	 *      the works chance of its condition (Config.GetWorksChance), otherwise it is dead.
	 *    - Otherwise it is Tested: bTested = true and bWorks = true. A dead item is never Tested on the table.
	 *  If there is no definition to roll (see RollDefinition), returns an item with Id INDEX_NONE and a null
	 *  Definition, and does not use up an Id.
	 */
	FFlipperItemInstance RollItem(FRandomStream& Random);

	/**
	 *  Rolls up to ItemsPerDay items and places each with FindFirstFit. Does not clear the table first.
	 *  If a rolled item does not fit anywhere, roll one replacement; if that does not fit either, skip that
	 *  item (so a crowded table ends up with fewer than ItemsPerDay items instead of looping forever).
	 *  Returns how many items were placed.
	 */
	int32 FillSellerTable(UInventoryGrid* Table, FRandomStream& Random);

private:
	UPROPERTY()
	TArray<TObjectPtr<UFlipperItemDefinition>> Definitions;

	UPROPERTY()
	TArray<TObjectPtr<UFlipperTraitDefinition>> Traits;

	FFlipperGenerationConfig Config;

	FFlipperPricingConfig Pricing;

	int32 NextItemId = 1;
};
