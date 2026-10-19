// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FlipperTypes.h"
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

	/** Chance (0 to 1) that an Untested item turns out to work. The rest are dead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = 0, ClampMax = 1))
	float UntestedWorksChance = 0.65f;
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
	/** Sets what can be rolled and with which numbers, and restarts the Id counter at 1. */
	void Init(const TArray<UFlipperItemDefinition*>& InDefinitions, const FFlipperGenerationConfig& InConfig);

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
	 *  Rolls one complete item with the next unused Id (1, 2, 3, ...): a definition, a rarity, and the
	 *  Tested / Untested state.
	 *    - Untested with chance UntestedChance. An Untested item has bTested = false, and bWorks is true with
	 *      chance UntestedWorksChance (otherwise it is dead).
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

	FFlipperGenerationConfig Config;

	int32 NextItemId = 1;
};
