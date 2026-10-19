// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperItemGenerator.h"
#include "FlipperItemDefinition.h"
#include "InventoryGrid.h"

void UFlipperItemGenerator::Init(const TArray<UFlipperItemDefinition*>& InDefinitions, const FFlipperGenerationConfig& InConfig)
{
	Definitions.Reset();
	for (UFlipperItemDefinition* Definition : InDefinitions)
	{
		Definitions.Add(Definition);
	}
	Config = InConfig;
	NextItemId = 1;
}

EFlipperRarity UFlipperItemGenerator::RollRarity(FRandomStream& Random) const
{
	// TODO: pick Common / Uncommon / Rare using Config.CommonWeight, UncommonWeight and RareWeight.
	// A weight of 0 must never come up; all three at 0 returns Common. Use Random, not FMath::Rand.
	float TotalWeight = Config.CommonWeight + Config.UncommonWeight + Config.RareWeight;

	if (TotalWeight == 0.f)
	{
		return EFlipperRarity::Common;
	}

	float RelativeCommonWeight = Config.CommonWeight / TotalWeight;
	float RelativeUncommonWeight = Config.UncommonWeight / TotalWeight;
	float RelativeRareWeight = Config.RareWeight / TotalWeight;
	float RandomFraction = Random.GetFraction();

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

	if (Random.GetFraction() < Config.UntestedChance)
	{
		// Untested: the works/dead flag is rolled now and stays hidden until the Garage test.
		Item.bTested = false;
		Item.bWorks = Random.GetFraction() < Config.UntestedWorksChance;
	}
	else
	{
		// Tested on the table means known working; a dead item only exists as an unlucky Untested buy.
		Item.bTested = true;
		Item.bWorks = true;
	}
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
