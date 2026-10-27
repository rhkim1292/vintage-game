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
	enum EClauseKind : int32
	{
		CategoryClause,
		TraitClause,
		MustWorkClause,
		ConditionClause,
		RarityClause,
		ClauseKindCount
	};

	// What Source can supply. Nothing is drawn here, so the pool can differ per source without moving the stream.
	TArray<FGameplayTag> CategoryTags;
	if (Source.Definition)
	{
		Source.Definition->Categories.GetGameplayTagArray(CategoryTags);
	}
	TArray<FGameplayTag> TraitTags;
	for (const UFlipperTraitDefinition* Trait : Source.Traits)
	{
		if (Trait && Trait->Tag.IsValid())
		{
			TraitTags.Add(Trait->Tag);
		}
	}

	const bool InPool[ClauseKindCount] = {
		!CategoryTags.IsEmpty(),
		!TraitTags.IsEmpty(),
		true,
		Source.Condition > EFlipperCondition::Poor,
		Source.Rarity > EFlipperRarity::Common,
	};
	int32 PoolSize = 0;
	for (const bool bAvailable : InPool)
	{
		PoolSize += bAvailable ? 1 : 0;
	}

	// All 8 numbers, in the header's order and taken on every path. RandHelper(0) would take none, so the picks use at least 1.
	const int32 CountIndex = FlipperRandom::RollWeightedIndex({ Config.OneClauseWeight, Config.TwoClausesWeight, Config.ThreeClausesWeight }, Random);
	const int32 CategoryPick = Random.RandHelper(FMath::Max(1, CategoryTags.Num()));
	const int32 TraitPick = Random.RandHelper(FMath::Max(1, TraitTags.Num()));
	float		SortKeys[ClauseKindCount];
	for (float& SortKey : SortKeys)
	{
		SortKey = Random.GetFraction();
	}

	// Index 0 is one clause. All-zero weights give INDEX_NONE: fall back to one clause. The pool always has must-work, so it is never empty.
	const int32 Wanted = FMath::Min(FMath::Max(1, CountIndex + 1), PoolSize);

	// Try the kinds from the lowest sort key up (ties by kind, so the order is always the same), and take the ones in the pool.
	TArray<int32> Order = { CategoryClause, TraitClause, MustWorkClause, ConditionClause, RarityClause };
	Order.Sort([&SortKeys](const int32 A, const int32 B) { return SortKeys[A] != SortKeys[B] ? SortKeys[A] < SortKeys[B] : A < B; });

	FFlipperRequest Request;
	int32			Taken = 0;
	for (const int32 Kind : Order)
	{
		if (Taken >= Wanted)
		{
			break;
		}
		if (!InPool[Kind])
		{
			continue;
		}

		switch (Kind)
		{
		case CategoryClause:
			Request.RequiredTags.AddTag(CategoryTags[CategoryPick]);
			break;
		case TraitClause:
			Request.RequiredTags.AddTag(TraitTags[TraitPick]);
			break;
		case MustWorkClause:
			Request.bMustWork = true;
			break;
		case ConditionClause:
			Request.MinCondition = Source.Condition;
			break;
		case RarityClause:
			Request.MinRarity = Source.Rarity;
			break;
		}
		++Taken;
	}
	return Request;
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
