// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperRequests.h"
#include "FlipperCollectorDefinition.h"
#include "FlipperItemDefinition.h"
#include "FlipperTraitDefinition.h"
#include "FlipperRandom.h"

bool FlipperRequests::Matches(const FFlipperItemInstance& Item, const FFlipperRequest& Request)
{
	// An item with no definition has no categories and matches nothing, not even an empty request.
	if (!Item.Definition)
	{
		return false;
	}

	// Must work means Tested AND working: an Untested item never counts, even if it happens to work.
	if (Request.bMustWork && !(Item.bTested && Item.bWorks))
	{
		return false;
	}

	// The enums are declared lowest first, so a plain comparison checks the minimums.
	if (Item.Condition < Request.MinCondition || Item.Rarity < Request.MinRarity)
	{
		return false;
	}

	// The item's tags: its categories plus the tag of each trait it carries.
	FGameplayTagContainer ItemTags = Item.Definition->Categories;
	for (const UFlipperTraitDefinition* Trait : Item.Traits)
	{
		if (Trait && Trait->Tag.IsValid())
		{
			ItemTags.AddTag(Trait->Tag);
		}
	}
	return ItemTags.HasAll(Request.RequiredTags);
}

FFlipperRequest FlipperRequests::RollRequest(const FFlipperItemInstance& Source, const FFlipperRequestConfig& Config, FRandomStream& Random)
{
	// TODO: build one request from Source (see the header for the pool, the clause count and the 8 numbers).
	// Hints: take all 8 numbers up front, in the order the header lists them, before looking at the pool: that is
	// what keeps the stream position the same on every path. FlipperRandom::RollWeightedIndex gives the clause count
	// (index 0 is one clause). For "which kinds, in what order", give each of the 5 kinds a random sort key, then walk
	// the kinds from the lowest key up and take the ones that are in the pool until you have the count.
	return FFlipperRequest();
}

int32 FlipperRequests::GetCollectorOffer(const FFlipperItemInstance& Item, const FFlipperRequest& Request, const FFlipperPricingConfig& Pricing, const FFlipperRequestConfig& Config, float MoodMultiplier)
{
	// Mood and premium go in as ONE multiplier, so GetOffer still rounds only once at the end.
	return FlipperPricing::GetOffer(Item, Pricing, MoodMultiplier * Config.GetPremium(Request.GetClauseCount()));
}

TArray<FFlipperCollector> FlipperRequests::RollBoard(const TArray<FFlipperItemInstance>& TableItems, const TArray<UFlipperCollectorDefinition*>& CollectorDefinitions, const FFlipperRequestConfig& Config, FRandomStream& Random)
{
	// Table items that can be a source (a request is built from a definition), and the collector names that can be given.
	TArray<int32> FreeSources;
	for (int32 Index = 0; Index < TableItems.Num(); ++Index)
	{
		if (TableItems[Index].Definition)
		{
			FreeSources.Add(Index);
		}
	}

	TArray<UFlipperCollectorDefinition*> AllNames;
	for (UFlipperCollectorDefinition* Name : CollectorDefinitions)
	{
		if (Name)
		{
			AllNames.Add(Name);
		}
	}
	TArray<UFlipperCollectorDefinition*> FreeNames = AllNames;

	TArray<FFlipperCollector> Board;
	const int32				  Count = FMath::Min(Config.CollectorsPerDay, FreeSources.Num());
	for (int32 Slot = 0; Slot < Count; ++Slot)
	{
		// One number to pick the source; RemoveAt (not RemoveAtSwap) keeps the order, so a seed always picks the same item.
		const int32 SourcePick = Random.RandHelper(FreeSources.Num());
		const int32 SourceIndex = FreeSources[SourcePick];
		FreeSources.RemoveAt(SourcePick);

		FFlipperCollector Collector;
		Collector.Request = RollRequest(TableItems[SourceIndex], Config, Random);

		// One number to pick the name, taken even with no names so the stream position does not depend on the data.
		if (FreeNames.IsEmpty())
		{
			FreeNames = AllNames;
		}
		const int32 NamePick = Random.RandHelper(FMath::Max(1, FreeNames.Num()));
		if (FreeNames.IsValidIndex(NamePick))
		{
			Collector.Definition = FreeNames[NamePick];
			FreeNames.RemoveAt(NamePick);
		}

		Board.Add(Collector);
	}
	return Board;
}
