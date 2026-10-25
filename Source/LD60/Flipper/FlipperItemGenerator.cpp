// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperItemGenerator.h"
#include "FlipperItemDefinition.h"
#include "InventoryGrid.h"

void UFlipperItemGenerator::Init(const TArray<UFlipperItemDefinition*>& InDefinitions, const FFlipperGenerationConfig& InConfig, const FFlipperPricingConfig& InPricing, const TArray<UFlipperTraitDefinition*>& InTraits)
{
	Definitions.Reset();
	for (UFlipperItemDefinition* Definition : InDefinitions)
	{
		Definitions.Add(Definition);
	}
	Traits.Reset();
	for (UFlipperTraitDefinition* Trait : InTraits)
	{
		Traits.Add(Trait);
	}
	Config = InConfig;
	Pricing = InPricing;
	NextItemId = 1;
}

EFlipperRarity UFlipperItemGenerator::RollRarity(FRandomStream& Random) const
{
	// TODO: pick Common / Uncommon / Rare using Config.CommonWeight, UncommonWeight and RareWeight.
	// A weight of 0 must never come up; all three at 0 returns Common. Use Random, not FMath::Rand.
	float TotalWeight = Config.CommonWeight + Config.UncommonWeight + Config.RareWeight;

	float RandomFraction = Random.GetFraction();

	if (TotalWeight == 0.f)
	{
		return EFlipperRarity::Common;
	}

	float RelativeCommonWeight = Config.CommonWeight / TotalWeight;
	float RelativeUncommonWeight = Config.UncommonWeight / TotalWeight;
	float RelativeRareWeight = Config.RareWeight / TotalWeight;

	if (RandomFraction <= RelativeRareWeight && RelativeRareWeight != 0.f)
	{
		return EFlipperRarity::Rare;
	}

	if (RandomFraction <= RelativeRareWeight + RelativeUncommonWeight && RelativeUncommonWeight != 0.f)
	{
		return EFlipperRarity::Uncommon;
	}

	if (Config.CommonWeight > 0.f)
	{
		return EFlipperRarity::Common;
	}

	// Only reachable through float rounding: give the leftover to the last tier that has weight.
	return Config.UncommonWeight > 0.f ? EFlipperRarity::Uncommon : EFlipperRarity::Rare;
}

EFlipperCondition UFlipperItemGenerator::RollCondition(FRandomStream& Random) const
{
	float TotalWeight = Config.MintWeight + Config.GoodWeight + Config.FairWeight + Config.PoorWeight;

	float RandomFraction = Random.GetFraction();

	if (TotalWeight == 0.f)
	{
		return EFlipperCondition::Good;
	}

	float RelativePoorWeight = Config.PoorWeight / TotalWeight;
	float RelativeFairWeight = Config.FairWeight / TotalWeight;
	float RelativeGoodWeight = Config.GoodWeight / TotalWeight;
	float RelativeMintWeight = Config.MintWeight / TotalWeight;

	if (RandomFraction <= RelativeMintWeight && RelativeMintWeight != 0.f)
	{
		return EFlipperCondition::Mint;
	}

	if (RandomFraction <= RelativeMintWeight + RelativeGoodWeight && RelativeGoodWeight != 0.f)
	{
		return EFlipperCondition::Good;
	}

	if (RandomFraction <= RelativeMintWeight + RelativeGoodWeight + RelativeFairWeight && RelativeFairWeight != 0.f)
	{
		return EFlipperCondition::Fair;
	}

	if (Config.PoorWeight > 0.f)
	{
		return EFlipperCondition::Poor;
	}

	// Only reachable through float rounding: give the leftover to the last tier that has weight.
	return Config.FairWeight > 0.f ? EFlipperCondition::Fair : Config.GoodWeight > 0.f ? EFlipperCondition::Good
																					   : EFlipperCondition::Mint;
}

namespace
{
	/**
	 *  Picks an index into Weights by relative weight, taking exactly one number from Random. A weight of 0 never wins.
	 *  Returns INDEX_NONE when no weight is above 0 (the number is still taken).
	 */
	int32 RollWeightedIndex(const TArray<float>& Weights, FRandomStream& Random)
	{
		float TotalWeight = 0.f;
		for (const float Weight : Weights)
		{
			TotalWeight += FMath::Max(Weight, 0.f);
		}

		// Same running-total walk as RollDefinition: each index owns the stretch [previous total, new total).
		const float Roll = Random.GetFraction() * TotalWeight;
		float		RunningTotal = 0.f;
		int32		LastUsable = INDEX_NONE;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			if (Weights[Index] <= 0.f)
			{
				continue;
			}
			RunningTotal += Weights[Index];
			LastUsable = Index;
			if (Roll < RunningTotal)
			{
				return Index;
			}
		}

		// Only reachable through float rounding (or with no weight at all): give the leftover to the last index that has weight.
		return LastUsable;
	}
} // namespace

TArray<TObjectPtr<UFlipperTraitDefinition>> UFlipperItemGenerator::RollTraits(FRandomStream& Random) const
{
	// Step 1: how many traits. The index that wins IS the count (0, 1 or 2). It always takes one number from Random,
	// and with every weight at 0 it gives INDEX_NONE, which means no traits.
	const int32 Count = FMath::Max(0, RollWeightedIndex({ Config.NoTraitsWeight, Config.OneTraitWeight, Config.TwoTraitsWeight }, Random));

	// Step 2: pick that many distinct traits. Only traits that can win are candidates. Each pick takes one number
	// and takes the winner out of the pool, so the same trait never comes up twice on one item.
	TArray<UFlipperTraitDefinition*> Candidates;
	TArray<float>					 Weights;
	for (const TObjectPtr<UFlipperTraitDefinition>& Trait : Traits)
	{
		if (Trait && Trait->RollWeight > 0.f)
		{
			Candidates.Add(Trait);
			Weights.Add(Trait->RollWeight);
		}
	}

	TArray<TObjectPtr<UFlipperTraitDefinition>> Picked;
	while (Picked.Num() < Count && !Candidates.IsEmpty())
	{
		const int32 Index = RollWeightedIndex(Weights, Random);
		if (Index == INDEX_NONE)
		{
			break;
		}
		Picked.Add(Candidates[Index]);
		Candidates.RemoveAt(Index);
		Weights.RemoveAt(Index);
	}
	return Picked;
}

UFlipperItemDefinition* UFlipperItemGenerator::RollDefinition(FRandomStream& Random) const
{
	// First pass: the total weight of the definitions that can win. Null or zero-weight entries never count.
	float TotalWeight = 0.f;
	for (const TObjectPtr<UFlipperItemDefinition>& Definition : Definitions)
	{
		if (Definition && Definition->SpawnWeight > 0.f)
		{
			TotalWeight += Definition->SpawnWeight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return nullptr;
	}

	// Scale the roll up to the total, then walk the list with a running total: each definition owns the
	// half-open stretch [previous total, new total), so a weight-0 entry owns nothing.
	const float				Roll = Random.GetFraction() * TotalWeight;
	float					RunningTotal = 0.f;
	UFlipperItemDefinition* LastUsable = nullptr;
	for (const TObjectPtr<UFlipperItemDefinition>& Definition : Definitions)
	{
		if (!Definition || Definition->SpawnWeight <= 0.f)
		{
			continue;
		}
		RunningTotal += Definition->SpawnWeight;
		LastUsable = Definition;
		if (Roll < RunningTotal)
		{
			return Definition;
		}
	}

	// Only reachable through float rounding: give the leftover to the last definition that has weight.
	return LastUsable;
}

FFlipperItemInstance UFlipperItemGenerator::RollItem(FRandomStream& Random)
{
	UFlipperItemDefinition* Definition = RollDefinition(Random);
	if (!Definition)
	{
		return FFlipperItemInstance();
	}

	FFlipperItemInstance Item;
	Item.Id = NextItemId++;
	Item.Definition = Definition;
	Item.Rarity = RollRarity(Random);
	Item.Condition = RollCondition(Random);

	if (Random.GetFraction() < Config.UntestedChance)
	{
		// Untested: the works/dead flag is rolled now and stays hidden until the Garage test.
		// The better the condition, the likelier it works.
		Item.bTested = false;
		Item.bWorks = Random.GetFraction() < Config.GetWorksChance(Item.Condition);
	}
	else
	{
		// Tested on the table means known working; a dead item only exists as an unlucky Untested buy.
		Item.bTested = true;
		Item.bWorks = true;
	}
	Item.Traits = RollTraits(Random);
	Item.AskPrice = FlipperPricing::GetAskPrice(Item, Pricing, Random);
	return Item;
}

int32 UFlipperItemGenerator::FillSellerTable(UInventoryGrid* Table, FRandomStream& Random)
{
	if (!Table)
	{
		return 0;
	}

	int32 Placed = 0;
	for (int32 Slot = 0; Slot < Config.ItemsPerDay; ++Slot)
	{
		// Two attempts per slot: the rolled item, then one replacement. Still no fit means the slot is skipped.
		for (int32 Attempt = 0; Attempt < 2; ++Attempt)
		{
			const FFlipperItemInstance Item = RollItem(Random);
			if (!Item.Definition)
			{
				break; // nothing can be rolled, so a replacement would be empty too
			}

			FIntPoint Origin;
			bool	  bRotated = false;
			if (Table->FindFirstFit(Item, Origin, bRotated) && Table->TryPlace(Item, Origin, bRotated))
			{
				++Placed;
				break;
			}
		}
	}
	return Placed;
}
